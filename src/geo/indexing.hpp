// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef GEO_INDEXING_HPP_
#define GEO_INDEXING_HPP_

#include <string>
#include <vector>

#include "btree/parallel_traversal.hpp"
#include "containers/counted.hpp"

namespace ql {
class datum_t;
}
class S2CellId;


std::vector<std::string> compute_index_grid_keys(
        const counted_t<const ql::datum_t> &key,
        int goal_cells);

// TODO! The next step is:
//  Somewhere in rdb_protocol, add a function that actually performs the
//  traversal using this helper.
//  When it gets a candidate, it should first extract the primary key
//  and see if it had previously handled that document. If not, it should load
//  the data and perform an intersection test on the actual geometry.
//  If the test is positive, it should store the primary key in the duplicates set.
//  It should also check the current size of that set and potentially abort the
//  traversal (ignore that at first).
// It should then emit the object with the data (since it has already loaded it).
// Something around it should then apply transformations etc.

// TODO! Support compound indexes. Somehow.
// TODO! Implement aborting the traversal early
class geo_index_traversal_helper_t : public btree_traversal_helper_t {
public:
    geo_index_traversal_helper_t(const std::vector<std::string> &query_grid_keys);

    /* Called for every pair that could potentially intersect with query_grid_keys.
    Note that this might be called multiple times for the same value. */
    virtual void on_candidate(
            const btree_key_t *key,
            const void *value)
            THROWS_ONLY(interrupted_exc_t) = 0;

    /* btree_traversal_helper_t interface */
    void process_a_leaf(
            buf_lock_t *leaf_node_buf,
            const btree_key_t *left_exclusive_or_null,
            const btree_key_t *right_inclusive_or_null,
            signal_t *interruptor,
            int *population_change_out)
            THROWS_ONLY(interrupted_exc_t);
    void postprocess_internal_node(UNUSED buf_lock_t *internal_node_buf) { }
    void filter_interesting_children(
            buf_parent_t parent,
            ranged_block_ids_t *ids_source,
            interesting_children_callback_t *cb);
    access_t btree_superblock_mode() { return access_t::read; }
    access_t btree_node_mode() { return access_t::read; }

private:
    static bool cell_intersects_with_range(
            S2CellId c, const btree_key_t *left_excl, const btree_key_t *right_incl);
    bool any_query_cell_intersects(const btree_key_t *left_excl,
                                   const btree_key_t *right_incl);

    std::vector<S2CellId> query_cells_;
};

#endif  // GEO_INDEXING_HPP_
