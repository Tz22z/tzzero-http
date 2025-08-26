#include "tzzero/core/poller.h"
#include "tzzero/core/epoll_poller.h"

#ifdef ENABLE_IO_URING
#include "tzzero/core/io_uring_poller.h"
#endif

#include <cstdlib>

namespace tzzero::core {

Poller::Poller(EventLoop* loop)
    : owner_loop_(loop)
{
}

std::unique_ptr<Poller> create_poller(EventLoop* loop) {
    // 检查环境变量以获取轮询器偏好
    const char* poller_type = std::getenv("TZZERO_POLLER");
    
#ifdef ENABLE_IO_URING
    if (poller_type && std::string(poller_type) == "epoll") {
        return std::make_unique<EpollPoller>(loop);
    }
    // 默认使用 io_uring（如果可用）
    return std::make_unique<IoUringPoller>(loop);
#else
    return std::make_unique<EpollPoller>(loop);
#endif
}

}  // namespace tzzero::core
