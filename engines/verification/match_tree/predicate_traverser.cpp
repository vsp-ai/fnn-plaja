//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "engines/verification/match_tree/predicate_traverser.h"

#ifdef LAZY_PA

#include "engines/verification/optimization/predicate_relations.h"
#include "engines/verification/pa_states/predicate_state.h"

PredicateTraverser::PredicateTraverser(const PAMatchTree& mt):
    matchTree(&mt)
    , currentNode(matchTree->root.get())
    , mtPrefix(new PredicateState(mt.get_size_predicates(), 0, &mt.get_predicates()))
    , closure(matchTree->get_global_predicates()/*currentNode->get_closure()*/) {
}

PredicateTraverser::~PredicateTraverser() = default;

/**********************************************************************************************************************/

inline void PredicateTraverser::push() {
    nodeStack.push_front(currentNode);

    closureStack.emplace_front(closure);

    if (mtPrefix) { mtPrefix->push_layer(); }
}

inline void PredicateTraverser::pop() {
    if (mtPrefix) { mtPrefix->pop_layer(); }

    closure = closureStack.front();
    closureStack.pop_front();

    PLAJA_ASSERT(not nodeStack.empty())
    currentNode = nodeStack.front();
    nodeStack.pop_front();
}

/**********************************************************************************************************************/

void PredicateTraverser::next(const PaStateBase& pa_state) {
    PLAJA_ASSERT(not end())

    push();

    PLAJA_ASSERT(pa_state.get_size_predicates() <= mtPrefix->get_size_predicates())
    PLAJA_ASSERT(pa_state.is_set(predicate_index()))

    const auto current_index = predicate_index();
    const bool truth_value = pa_state.predicate_value(current_index);

    if (not currentNode or currentNode->get_predicate_index() == PREDICATE::none) {

        currentNode = nullptr;

        closure.pop_front(); // index is handled, one way or the other

    }

    PLAJA_ASSERT(not mtPrefix->is_entailed(current_index))

    mtPrefix->set(current_index, truth_value);

    if (matchTree->get_predicate_relations()) { // current value is not entailed so let's update entailment ...

        matchTree->get_predicate_relations()->fix_predicate_state_asc_if_non_lazy(*mtPrefix, current_index);

    }

    while (not closure.empty() and mtPrefix->is_defined(closure.front())) { closure.pop_front(); }

    if (not currentNode) { return; } // possible that state has not been added to MT so far ...

    currentNode = currentNode->access_successor(truth_value);

}

void PredicateTraverser::previous() { pop(); }

bool PredicateTraverser::has_node() const { return currentNode; /* and not mtPrefix->is_entailed(predicate_index());*/ }

/**********************************************************************************************************************/

#ifndef NDEBUG

bool PredicateTraverser::is_closed(const PAMatchTree& mt, const PaStateBase& pa_state, bool minimal) {

    auto pred_traverser = init(mt);

    std::size_t closure_size = 0;

    while (not pred_traverser.end()) {

        if (pred_traverser.traversed(pa_state)) { return false; }

        pred_traverser.next(pa_state);

        ++closure_size;
    }

    if (minimal) {
        if (closure_size != pa_state.get_number_of_set_predicates()) { return false; }
    } else { PLAJA_ASSERT(closure_size <= pa_state.get_size_predicates()) }

    return true;

}

#endif

#endif