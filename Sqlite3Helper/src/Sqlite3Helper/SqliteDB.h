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
        // �������ݿ⡢����ͬ��ģʽ��������־ģʽ
        bool InitDataBase(const std::string& dataBaseFullPath, const bool& isWal = true, const SynchronousLevel& synchronousLevel = SynchronousLevel::FULL);
        bool ExecuteBatch(const std::string& querySqlCmd, std::list<Binder>* pBinderList, std::list<Parser>* pParserList);
    protected:
        // ����ʵ�ָ��ԵĴ�����sqlite3_exec
        virtual bool CreateTabel(sqlite3* db) = 0;
    protected:
        /* ����g_db��thread_local�������������Σ����ԣ�
         * ConnectDB()�����ж�g_db�ĵ�ַ��������ÿ��sql�����ж��ȵ��ã���ֹg_dbδ����ʹ�ñ���
         * DisConnectDB()������g_db���߳�Ψһ�������������߳̽���ʱ������ã���ֹ
        */
        bool ConnectDB();
        bool DisConnectDB();
        // ��������ʽ����-ʹ��Ĭ�ϣ�д-ʹ��IMEMEDATE
        bool BeginTransaction(const TransactionLevel& transactionLevel = TransactionLevel::DEFERRED);
        bool RollbackTransaction();
        bool CommitTransaction();

        void CatchHandler(ThrowExceptionDefine& errorCode);

    protected:
        // ����ģʽ�£���֤ÿ�����ݿ������ͬһʱ��ֻ��һ���߳���ʹ��
        static thread_local sqlite3* g_db;
        static thread_local sqlite3_stmt* g_stmt;

    private:
        std::string m_dataBaseFullPath;
    };

}

#endif // !SQLITE3_DB_H
