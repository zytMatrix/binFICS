#include "PointerSubgraph.h"

namespace dg {
namespace analysis {
namespace pta {

// create subgraph or add edges to already existing subgraph,
// return the CALL node (the first) and the RETURN node (the second),
// so that we can connect them into the PointerSubgraph
PSNodesSeq
LLVMPointerSubgraphBuilder::createCall(const llvm::Instruction *Inst)
{
    using namespace llvm;
    const CallInst *CInst = cast<CallInst>(Inst);
    const Value *calledVal = CInst->getCalledValue()->stripPointerCasts();

    if (CInst->isInlineAsm()) {
        PSNode *n = createAsm(Inst);
        return std::make_pair(n, n);
    }

    if (const Function *func = dyn_cast<Function>(calledVal)) {
        return createFunctionCall(CInst, func);
    } else {
        // this is a function pointer call
        return createFuncptrCall(CInst, calledVal);
    }
}


PSNodesSeq
LLVMPointerSubgraphBuilder::createFunctionCall(const llvm::CallInst *CInst, const llvm::Function *func)
{
    // is it a call to free? If so, create invalidate node instead.
    if(invalidate_nodes && func->getName().equals("free")) {
        PSNode *n = createFree(CInst);
        return std::make_pair(n, n);
    }
    
    // is function undefined? If so it can be
    // intrinsic, memory allocation (malloc, calloc,...)
    // or just undefined function
    // NOTE: we first need to check whether the function
    // is undefined and after that if it is memory allocation,
    // because some programs may define function named
    // 'malloc' etc.
    if (func->size() == 0) {
        /// memory allocation (malloc, calloc, etc.)
        MemAllocationFuncs type = getMemAllocationFunc(func);
        if (type != MemAllocationFuncs::NONEMEM) {
            return createDynamicMemAlloc(CInst, type);
        } else if (func->isIntrinsic()) {
            return createIntrinsic(CInst);
        } else
            return createUnknownCall(CInst);
    }

    auto seq = createCallToFunction(CInst, func);
    addNode(CInst, seq.first);

    return seq;
}

PSNodesSeq
LLVMPointerSubgraphBuilder::createFuncptrCall(const llvm::CallInst *CInst, const llvm::Value *calledVal)
{
        // just the call_funcptr and call_return nodes are created and
        // when the pointers are resolved during analysis, the graph
        // will be dynamically created and it will replace these nodes
        PSNode *op = getOperand(calledVal);
        PSNode *call_funcptr = PS.create(PSNodeType::CALL_FUNCPTR, op);
        PSNode *ret_call = PS.create(PSNodeType::CALL_RETURN, nullptr);

        ret_call->setPairedNode(call_funcptr);
        call_funcptr->setPairedNode(ret_call);

        call_funcptr->addSuccessor(ret_call);
        addNode(CInst, call_funcptr);

        return std::make_pair(call_funcptr, ret_call);
}

PSNodesSeq
LLVMPointerSubgraphBuilder::createUnknownCall(const llvm::CallInst *CInst)
{
    // This assertion must not hold if the call is wrapped
    // inside bitcast - it defaults to int, but is bitcased
    // to pointer
    //assert(CInst->getType()->isPointerTy());
    PSNode *call = PS.create(PSNodeType::CALL, nullptr);

    call->setPairedNode(call);

    // the only thing that the node will point at
    call->addPointsTo(PointerUnknown);

    addNode(CInst, call);

    return std::make_pair(call, call);
}

PSNode *LLVMPointerSubgraphBuilder::createMemTransfer(const llvm::IntrinsicInst *I)
{
    using namespace llvm;
    const Value *dest, *src;//, *lenVal;
    uint64_t lenVal = Offset::UNKNOWN;

    switch (I->getIntrinsicID()) {
        case Intrinsic::memmove:
        case Intrinsic::memcpy:
            dest = I->getOperand(0);
            src = I->getOperand(1);
            lenVal = getConstantValue(I->getOperand(2));
            break;
        default:
            errs() << "ERR: unhandled mem transfer intrinsic" << *I << "\n";
            abort();
    }

    PSNode *destNode = getOperand(dest);
    PSNode *srcNode = getOperand(src);
    PSNode *node = PS.create(PSNodeType::MEMCPY,
                              srcNode, destNode, lenVal);

    addNode(I, node);
    return node;
}

PSNodesSeq
LLVMPointerSubgraphBuilder::createMemSet(const llvm::Instruction *Inst)
{
    PSNode *val;
    if (memsetIsZeroInitialization(llvm::cast<llvm::IntrinsicInst>(Inst)))
        val = NULLPTR;
    else
        // if the memset is not 0-initialized, it does some
        // garbage into the pointer
        val = UNKNOWN_MEMORY;

    PSNode *op = getOperand(Inst->getOperand(0)->stripInBoundsOffsets());
    // we need to make unknown offsets
    PSNode *G = PS.create(PSNodeType::GEP, op, Offset::UNKNOWN);
    PSNode *S = PS.create(PSNodeType::STORE, val, G);
    G->addSuccessor(S);

    PSNodesSeq ret = PSNodesSeq(G, S);
    addNode(Inst, ret);

    return ret;
}

PSNodesSeq
LLVMPointerSubgraphBuilder::createVarArg(const llvm::IntrinsicInst *Inst)
{
    // just store all the pointers from vararg argument
    // to the memory given in vastart() on Offset::UNKNOWN.
    // It is the easiest thing we can do without any further
    // analysis

    // first we need to get the vararg argument phi
    const llvm::Function *F = Inst->getParent()->getParent();
    Subgraph& subg = subgraphs_map[F];
    PSNode *arg = subg.vararg;
    assert(F->isVarArg() && "vastart in a non-variadic function");
    assert(arg && "Don't have variadic argument in a variadic function");

    // vastart will be node that will keep the memory
    // with pointers, its argument is the alloca, that
    // alloca will keep pointer to vastart
    PSNode *vastart = PS.create(PSNodeType::ALLOC);

    // vastart has only one operand which is the struct
    // it uses for storing the va arguments. Strip it so that we'll
    // get the underlying alloca inst
    PSNode *op = getOperand(Inst->getOperand(0)->stripInBoundsOffsets());
    // the argument is usually an alloca, but it may be a load
    // in the case the code was transformed by -reg2mem
    assert((op->getType() == PSNodeType::ALLOC || op->getType() == PSNodeType::LOAD)
           && "Argument of vastart is invalid");
    // get node with the same pointer, but with Offset::UNKNOWN
    // FIXME: we're leaking it
    // make the memory in alloca point to our memory in vastart
    PSNode *ptr = PS.create(PSNodeType::GEP, op, Offset::UNKNOWN);
    PSNode *S1 = PS.create(PSNodeType::STORE, vastart, ptr);
    // and also make vastart point to the vararg args
    PSNode *S2 = PS.create(PSNodeType::STORE, arg, vastart);

    vastart->addSuccessor(ptr);
    ptr->addSuccessor(S1);
    S1->addSuccessor(S2);

    // set paired node to S2 for vararg, so that when adding structure,
    // we add the whole sequence (it adds from call-node to pair-node,
    // because of the old system where we did not store all sequences)
    // FIXME: fix this
    vastart->setPairedNode(S2);

    // FIXME: we're assuming that in a sequence in the nodes_map
    // is always the last node the 'real' node. In this case it is not true,
    // so add only the 'vastart', so that we have the mapping in nodes_map
    addNode(Inst, vastart);

    return PSNodesSeq(vastart, S2);
}

PSNodesSeq
LLVMPointerSubgraphBuilder::createIntrinsic(const llvm::Instruction *Inst)
{
    using namespace llvm;
    PSNode *n;

    const IntrinsicInst *I = cast<IntrinsicInst>(Inst);
    if (isa<MemTransferInst>(I)) {
        n = createMemTransfer(I);
        return std::make_pair(n, n);
    } else if (isa<MemSetInst>(I)) {
        return createMemSet(I);
    }

    switch (I->getIntrinsicID()) {
        case Intrinsic::vastart:
            return createVarArg(I);
        case Intrinsic::stacksave:
            errs() << "WARNING: Saving stack may yield unsound results!: "
                   << *Inst << "\n";
            n = createAlloc(Inst);
            return std::make_pair(n, n);
        case Intrinsic::stackrestore:
            n = createLoad(Inst);
            return std::make_pair(n, n);
        default:
            errs() << *Inst << "\n";
            errs() << "Unhandled intrinsic ^^\n";
            abort();
    }
}

PSNode *
LLVMPointerSubgraphBuilder::createAsm(const llvm::Instruction *Inst)
{
    // we filter irrelevant calls in isRelevantCall()
    // and we don't have assembler there at all. If
    // we are here, then we got here because this
    // is undefined call that returns pointer.
    // In this case return an unknown pointer
    static bool warned = false;
    if (!warned) {
        llvm::errs() << "PTA: Inline assembly found, analysis  may be unsound\n";
        warned = true;
    }

    PSNode *n = PS.create(PSNodeType::CONSTANT, UNKNOWN_MEMORY, Offset::UNKNOWN);
    // it is call that returns pointer, so we'd like to have
    // a 'return' node that contains that pointer
    n->setPairedNode(n);
    addNode(Inst, n);

    return n;
}

PSNode * LLVMPointerSubgraphBuilder::createFree(const llvm::Instruction *Inst)
{
    PSNode *op1 = getOperand(Inst->getOperand(0));
    PSNode *node = PS.create(PSNodeType::FREE, op1);

    addNode(Inst, node);

    assert(node);
    return node;
}

PSNode *LLVMPointerSubgraphBuilder::createDynamicAlloc(const llvm::CallInst *CInst, MemAllocationFuncs type)
{
    using namespace llvm;

    const Value *op;
    uint64_t size = 0, size2 = 0;
    PSNodeAlloc *node = PSNodeAlloc::get(PS.create(PSNodeType::DYN_ALLOC));

    switch (type) {
        case MemAllocationFuncs::MALLOC:
            node->setIsHeap();
            /* fallthrough */
        case MemAllocationFuncs::ALLOCA:
            op = CInst->getOperand(0);
            break;
        case MemAllocationFuncs::CALLOC:
            node->setIsHeap();
            node->setZeroInitialized();
            op = CInst->getOperand(1);
            break;
        default:
            errs() << *CInst << "\n";
            assert(0 && "unknown memory allocation type");
            // for NDEBUG
            abort();
    };

    // infer allocated size
    size = getConstantSizeValue(op);
    if (size != 0 && type == MemAllocationFuncs::CALLOC) {
        // if this is call to calloc, the size is given
        // in the first argument too
        size2 = getConstantSizeValue(CInst->getOperand(0));
        if (size2 != 0)
            size *= size2;
    }

    node->setSize(size);
    return node;
}

PSNodesSeq
LLVMPointerSubgraphBuilder::createRealloc(const llvm::CallInst *CInst)
{
    using namespace llvm;

    // we create new allocation node and memcpy old pointers there
    PSNode *orig_mem = getOperand(CInst->getOperand(0));
    PSNodeAlloc *reall = PSNodeAlloc::get(PS.create(PSNodeType::DYN_ALLOC));
    // copy everything that is in orig_mem to reall
    PSNode *mcp = PS.create(PSNodeType::MEMCPY, orig_mem, reall, Offset::UNKNOWN);
    // we need the pointer in the last node that we return
    PSNode *ptr = PS.create(PSNodeType::CONSTANT, reall, 0);

    reall->setIsHeap();
    reall->setSize(getConstantSizeValue(CInst->getOperand(1)));

    reall->addSuccessor(mcp);
    mcp->addSuccessor(ptr);

    reall->setUserData(const_cast<llvm::CallInst *>(CInst));

    PSNodesSeq ret = PSNodesSeq(reall, ptr);
    addNode(CInst, ret);

    return ret;
}

PSNodesSeq
LLVMPointerSubgraphBuilder::createDynamicMemAlloc(const llvm::CallInst *CInst, MemAllocationFuncs type)
{
    assert(type != MemAllocationFuncs::NONEMEM
            && "BUG: creating dyn. memory node for NONMEM");

    if (type == MemAllocationFuncs::REALLOC) {
        return createRealloc(CInst);
    } else {
        PSNode *node = createDynamicAlloc(CInst, type);
        addNode(CInst, node);

        // we return (node, node), so that the parent function
        // will seamlessly connect this node into the graph
        return std::make_pair(node, node);
    }
}



} // namespace pta
} // namespace analysis
} // namespace dg

