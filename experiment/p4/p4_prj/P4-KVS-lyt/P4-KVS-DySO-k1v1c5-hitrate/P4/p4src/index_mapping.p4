#ifndef _INDEX_MAPPING_P4_
#define _INDEX_MAPPING_P4_


#define INDEX_MAPPING(lock_id, index)                   \
    action mapping(bit<IDX_WIDTH> mapped_index)         \
    {                                                   \
        index = mapped_index;                           \
    }                                                   \
    action hard_code_mapping()                          \
    {                                                   \
        index = lock_id[IDX_WIDTH - 1:0];               \
    }                                                   \
    table index_mapping_table                           \
    {                                                   \
        key = {                                         \
            lock_id : range;                            \
        }                                               \
        actions = {                                     \
            mapping;                                    \
            hard_code_mapping;                          \
            NoAction;                                   \
        }                                               \
        size = 65536;                                   \
        const entries = {                               \
            (0 .. SLOT_SIZE - 1) : hard_code_mapping(); \
        }                                               \
        default_action = NoAction;                      \
    }

control IndexMapping(
    in  bit<32> lock_id,
    out bit<IDX_WIDTH> index,
    out bit<1> is_hit
)
{
    action mapping(bit<IDX_WIDTH> mapped_index)
    {
        index = mapped_index;
    }

    table index_mapping_table
    {
        key = {
            lock_id : exact;
        }

        actions = {
            mapping;
            NoAction;
        }

        size = SLOT_SIZE;
    }

    apply
    {
        is_hit = mapping.apply().hit
    }
}


#endif //_INDEX_MAPPING_P4_
