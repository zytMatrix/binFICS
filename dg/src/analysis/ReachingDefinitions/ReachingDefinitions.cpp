#include <set>

#include "RDMap.h"
#include "ReachingDefinitions.h"

namespace dg {
namespace analysis {
namespace rd {

RDNode UNKNOWN_MEMLOC;
RDNode *UNKNOWN_MEMORY = &UNKNOWN_MEMLOC;

bool ReachingDefinitionsAnalysis::processNode(RDNode *node)
{
    bool changed = false;

    // merge maps from predecessors
    for (RDNode *n : node->predecessors)
        changed |= node->def_map.merge(&n->def_map,
                                       &node->overwrites /* strong update */,
                                       strong_update_unknown,
                                       max_set_size /* max size of set of reaching definition
                                                       of one definition site */,
                                       false /* merge unknown */);

    return changed;
}

void ReachingDefinitionsAnalysis::run()
{
    assert(root && "Do not have root");

    std::vector<RDNode *> to_process = getNodes(root);
    std::vector<RDNode *> changed;

    // do fixpoint
    do {
        unsigned last_processed_num = to_process.size();
        changed.clear();

        for (RDNode *cur : to_process) {
            if (processNode(cur))
                changed.push_back(cur);
        }

        if (!changed.empty()) {
            to_process.clear();
            to_process = getNodes(nullptr /* starting node */,
                                  &changed /* starting set */,
                                  last_processed_num /* expected num */);

            // since changed was not empty,
            // the to_process must not be empty too
            assert(!to_process.empty());
        }
    } while (!changed.empty());
}


} // namespace rd
} // namespace analysis
} // namespace dg
