#include <iostream>
#include <string>

class User {
public:
    virtual ~User() {}

    // 带默认实现的虚拟函数
    virtual void process_query() {
        std::cout << "Default process_query: target_hit_index = " << target_hit_index 
                  << ", target_obj = " << target_obj << std::endl;
    }

    virtual void datamv_server2switch() {
        std::cout << "Default datamv_server2switch operation." << std::endl;
    }

    virtual void datamv_switch2server() {
        std::cout << "Default datamv_switch2server operation." << std::endl;
    }
};
