#include "Async.h"

#include <future>
#include <list>
#include <memory>

namespace
{
class FeatureWrapper;
class FuturesHandler
{
public:
    ~FuturesHandler();
    void append(const std::shared_ptr<FeatureWrapper>& handler);
    void remove(const std::shared_ptr<FeatureWrapper>& handler);
    void wait();
private:
    std::mutex _mutex;
    std::list<std::shared_ptr<FeatureWrapper>> _futures;
};

class FeatureWrapper : public std::enable_shared_from_this<FeatureWrapper>
{
public:
    void setFuture(std::future<void>&& future) {
        _future = std::move(future);
    }

    void attach(FuturesHandler* handler)
    {
        _handler = handler;
    }

    void detach()
    {
        _handler->remove(shared_from_this());
    }

    void wait()
    {
        _future.wait();
    }

private:
    std::future<void> _future;
    FuturesHandler*_handler = nullptr;
};

FuturesHandler::~FuturesHandler()
{
    wait();
}

void FuturesHandler::append(const std::shared_ptr<FeatureWrapper> &handler)
{
    const std::lock_guard<std::mutex> lock(_mutex);
    handler->attach(this);
    _futures.push_back(handler);
}

void FuturesHandler::remove(const std::shared_ptr<FeatureWrapper> &handler)
{
    const std::lock_guard<std::mutex> lock(_mutex);
    _futures.remove(handler);
}

void FuturesHandler::wait()
{
    const auto futures = [this]() {
        const std::lock_guard<std::mutex> lock(_mutex);
        return std::move(_futures);
    }();

    for (const auto& futureWrapper : futures)
        futureWrapper->wait();
}

}

namespace Async
{
    FuturesHandler &futuresHandlerInstance()
    {
        static FuturesHandler instance;
        return instance;
    }

    void WaitAllTasksFinished()
    {
        futuresHandlerInstance().wait();
    }

    void Run(std::function<void()> &&func)
    {
        const auto handler = std::make_shared<FeatureWrapper>();
        futuresHandlerInstance().append(handler);
        auto future = std::async(std::launch::async, [f = std::forward<decltype(func)>(func), handler](){
            f();
            handler->detach();
        });
        handler->setFuture(std::move(future));
    }
}