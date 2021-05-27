#ifndef _DG_ANALYSIS_POINTS_TO_FLOW_SENSITIVE_H_
#define _DG_ANALYSIS_POINTS_TO_FLOW_SENSITIVE_H_

#include <cassert>
#include <memory>

#include "MemoryObject.h"
#include "PointerSubgraph.h"

namespace dg {
namespace analysis {
namespace pta {

class PointsToFlowSensitive : public PointerAnalysis
{
public:
    //using MemoryObjectsSetT = std::set<MemoryObject *>;
    using MemoryMapT = std::map<PSNode *, std::unique_ptr<MemoryObject>>;

    // this is an easy but not very efficient implementation,
    // works for testing
    PointsToFlowSensitive(PointerSubgraph *ps, bool invalidate = false)
    : PointerAnalysis(ps, Offset::UNKNOWN, false, invalidate)
    {
        memoryMaps.reserve(ps->size() / 5);
    }

    bool beforeProcessed(PSNode *n) override
    {
        MemoryMapT *mm = n->getData<MemoryMapT>();
        if (mm)
            return false;

        // on these nodes the memory map can change
        if (needsMerge(n)) { // root node
            mm = createMM();
        } else {
            // this node can not change the memory map,
            // so just add a pointer from the predecessor
            // to this map
            PSNode *pred = n->getSinglePredecessor();
            mm = pred->getData<MemoryMapT>();
            assert(mm && "No memory map in the predecessor");
        }

        assert(mm && "Did not create the MM");

        // memory map initialized, set it as data,
        // so that we won't initialize it again
        n->setData<MemoryMapT>(mm);

        return true;
    }

    bool afterProcessed(PSNode *n) override
    {
        bool changed = false;
        PointsToSetT *strong_update = nullptr;

        MemoryMapT *mm = n->getData<MemoryMapT>();
        // we must have the memory map, we created it
        // in the beforeProcessed method
        assert(mm && "Do not have memory map");

        // every store is a strong update
        // FIXME: memcpy can be strong update too
        if (n->getType() == PSNodeType::STORE)
            strong_update = &n->getOperand(1)->pointsTo;

        // merge information from predecessors if there's
        // more of them (if there's just one predecessor
        // and this is not a store, the memory map couldn't
        // change, so we don't have to do that)
        if (needsMerge(n)) {
            for (PSNode *p : n->getPredecessors()) {
                MemoryMapT *pm = p->getData<MemoryMapT>();
                // merge pm to mm (but only if pm was already created)
                if (pm) {
                    changed |= mergeMaps(mm, pm, strong_update);
                }
            }
        }

        return changed;
    }

    void getMemoryObjects(PSNode *where, const Pointer& pointer,
                          std::vector<MemoryObject *>& objects) override
    {
        MemoryMapT *mm = where->getData<MemoryMapT>();
        assert(mm && "Node does not have memory map");

        auto I = mm->find(pointer.target);
        if (I != mm->end()) {
            objects.push_back(I->second.get());
        }

        // if we haven't found any memory object, but this psnode
        // is a write to memory, create a new one, so that
        // the write has something to write to
        if (objects.empty() && canChangeMM(where)) {
            MemoryObject *mo = new MemoryObject(pointer.target);
            mm->emplace(pointer.target, std::unique_ptr<MemoryObject>(mo));
            objects.push_back(mo);
        }
    }

protected:

    PointsToFlowSensitive() = default;

    static bool canChangeMM(PSNode *n) {
        if (n->predecessorsNum() == 0) // root node
            return true;

        switch (n->getType()) {
            case PSNodeType::STORE:
            case PSNodeType::MEMCPY:
            case PSNodeType::CALL_FUNCPTR:
                // a call via function pointer needs to
                // have its own memory map as we dont know
                // how the graph will look like after the
                // call yet
                return true;
            case PSNodeType::CALL_RETURN:
                // return from function that was called via function
                // pointer must have its own memory map from the
                // same reason why CALL_FUNCPTR nodes need its
                // own memory map
                assert(n->getPairedNode());
                return n->getPairedNode()->getType() == PSNodeType::CALL_FUNCPTR;
            default:
                return false;
            }

            return false;
    }

    static bool mergeObjects(PSNode *node,
                             MemoryObject *to,
                             MemoryObject *from,
                             PointsToSetT *strong_update) {
        bool changed = false;

        for (auto& fromIt : from->pointsTo) {
            if (strong_update &&
                strong_update->count(Pointer(node, fromIt.first)))
                continue;

            auto& S = to->pointsTo[fromIt.first];
            for (const auto& ptr : fromIt.second)
                changed |= S.add(ptr);
        }

        return changed;
    }

    // Merge two Memory maps, return true if any new information was created,
    // otherwise return false
    static bool mergeMaps(MemoryMapT *mm, MemoryMapT *from,
                          PointsToSetT *strong_update) {
        bool changed = false;
        for (auto& it : *from) {
            PSNode *fromTarget = it.first;
            std::unique_ptr<MemoryObject>& toMo = (*mm)[fromTarget];
            if (toMo == nullptr)
                toMo.reset(new MemoryObject(fromTarget));

            changed |= mergeObjects(fromTarget, toMo.get(),
                                    it.second.get(), strong_update);
        }

        return changed;
    }

    MemoryMapT *createMM() {
        MemoryMapT *mm = new MemoryMapT();
        memoryMaps.emplace_back(mm);
        return mm;
    }

private:
    static bool needsMerge(PSNode *n) {
        return n->predecessorsNum() > 1 || canChangeMM(n);
    }

    // keep all the maps in order to free the memory
    std::vector<std::unique_ptr<MemoryMapT>> memoryMaps;
};

} // namespace pta
} // namespace analysis
} // namespace dg

#endif // _DG_ANALYSIS_POINTS_TO_FLOW_SENSITIVE_H_
