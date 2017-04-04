#include "resource_manager.hpp"

bool resource::is_processed(resource::types type)
{
    return type >= resource::unprocessed_end;
}

resource_manager::resource_manager()
{
    resources.resize(resource::types::COUNT);
}

resource_element& resource_manager::get_resource(resource::types type)
{
    for(auto& i : resources)
    {
        if(i.type == type)
            return i;
    }

    throw;
}
