#ifndef GLOBAL_FUNCTION_H
#define GLOBAL_FUNCTION_H
#include <functional>
#include <thread>
#include <vector>

namespace Common {
    using ExitCallBack = std::function<bool(void)>;
    void OnExitThread(const std::vector<ExitCallBack>& exitCallBacks) {
        class ThreadExiter {
        public:
            ~ThreadExiter()
            {
                for (auto func : m_exitCallBacks) {
                    func();
                }
            };

            void AddFunc(const ExitCallBack& exitCallBack) {
                m_exitCallBacks.push_back(exitCallBack);
            }
            void AddFunc(const std::vector<ExitCallBack>& exitCallBacks) {
                m_exitCallBacks.assign(exitCallBacks.begin(), exitCallBacks.end());
            }

        public:
            ThreadExiter() = default;
            ThreadExiter(ThreadExiter const&) = delete;
            void operator=(ThreadExiter const&) = delete;
        private:
            std::vector<ExitCallBack> m_exitCallBacks;
        };

        thread_local ThreadExiter exiter;
        if (exitCallBacks.size() == 1) {
            exiter.AddFunc(exitCallBacks[0]);
        } else {
            exiter.AddFunc(exitCallBacks);
        }
    }
    void OnExitThread(const ExitCallBack& exitCallBack) {
        class ThreadExiter {
        public:
            ~ThreadExiter()
            {
                m_exitCallBack();
            };

            void AddFunc(const ExitCallBack& exitCallBack) {
                m_exitCallBack = exitCallBack;
            }

        public:
            ThreadExiter() = default;
            ThreadExiter(ThreadExiter const&) = delete;
            void operator=(ThreadExiter const&) = delete;
        private:
            ExitCallBack m_exitCallBack;
        };

        thread_local ThreadExiter exiter;
        exiter.AddFunc(exitCallBack);
    }
}

#endif // !GLOBAL_FUNCTION_H

