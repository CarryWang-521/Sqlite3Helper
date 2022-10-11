#ifndef SQLITE3_DB_H
#define SQLITE3_DB_H

#include <string>
#include <thread>
#include <vector>
#include <list>
#include <functional>
#include "sqlite3.h"
#include "BinderAndParser.h"

namespace CommonSqlite {
    enum class SynchronousLevel : int {
        OFF,
        NORMAL,
        FULL,
    };
    enum class TransactionLevel : int {
        DEFERRED,
        IMMEDIATE,
        EXCLUSIVE,
    };
    enum class ThrowExceptionDefine : int {
        SQLITE3_OPEN_V2,
        SQLITE3_BUSY_HANDLER,
        SQLITE3_PRAGMA_JOURNAL_MODE_WAL,
        SQLITE3_PRAGMA_SYNCHRONOUS_OFF,
        SQLITE3_PRAGMA_SYNCHRONOUS_NORMAL,
        SQLITE3_PRAGMA_SYNCHRONOUS_FULL,
        SQLITE3_EXEC_CREATE_TABLE,
        SQLITE3_EXEC_BEGIN,
        SQLITE3_EXEC_ROLLBACK,
        SQLITE3_EXEC_COMMIT,
        SQLITE3_PREPARE_V2,
        SQLITE3_RESET,
        SQLITE3_BIND,
        SQLITE3_STEP,
        SQLITE3_COLUMN_xxx,
        SQLITE3_FINALIZE,
        SQLITE3_CLOSE_V2,
    };

    class SqliteDB
    {
    public:
        SqliteDB() = default;
        virtual ~SqliteDB() = default;

    public:
        // 创建数据库、设置同步模式、设置日志模式
        bool InitDataBase(const std::string& dataBaseFullPath, const bool& isWal = true, const SynchronousLevel& synchronousLevel = SynchronousLevel::FULL);
        bool ExecuteBatch(const std::string& querySqlCmd, std::list<Binder>* pBinderList, std::list<Parser>* pParserList);
    protected:
        // 子类实现各自的创建表sqlite3_exec
        virtual bool CreateTabel(sqlite3* db) = 0;
    protected:
        /* 由于g_db是thread_local变量声明符修饰，所以：
         * ConnectDB()：会判断g_db的地址，所以在每个sql方法中都先调用，防止g_db未连接使用报错。
         * DisConnectDB()：由于g_db是线程唯一，所以我们在线程结束时必须调用，防止
        */
        bool ConnectDB();
        bool DisConnectDB();
        // 事务开启方式：读-使用默认；写-使用IMEMEDATE
        bool BeginTransaction(const TransactionLevel& transactionLevel = TransactionLevel::DEFERRED);
        bool RollbackTransaction();
        bool CommitTransaction();

        void CatchHandler(ThrowExceptionDefine& errorCode);

    protected:
        // 并发模式下，保证每个数据库的连接同一时刻只在一个线程中使用
        static thread_local sqlite3* g_db;
        static thread_local sqlite3_stmt* g_stmt;

    private:
        std::string m_dataBaseFullPath;
    };

}

#endif // !SQLITE3_DB_H
