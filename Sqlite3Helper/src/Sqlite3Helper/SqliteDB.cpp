#include "SqliteDB.h"
#include <iostream>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif
#include "GlobalFunction.h"


namespace CommonSqlite {
    thread_local sqlite3* SqliteDB::g_db = nullptr;
    thread_local sqlite3_stmt* SqliteDB::g_stmt = nullptr;

    int BusyCallBack(void* db, int param) {
        sqlite3* pDb = static_cast<sqlite3*>(db);
        std::cout << "BusyCallBack: [g_db:" << std::hex << pDb << "]" << std::endl;
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        return 1;
    }

    bool SqliteDB::InitDataBase(const std::string& dataBaseFullPath, const bool& isWal, const SynchronousLevel& synchronousLevel)
    {
        m_dataBaseFullPath = dataBaseFullPath;
        sqlite3* db = nullptr;
        char* zErrMsg = nullptr;
        try {
            int result = sqlite3_open_v2(m_dataBaseFullPath.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, nullptr);
            if (result != SQLITE_OK) {
                std::cout << "[SqliteDB][InitDataBase][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] sqlite3_open_v2 failed: " << sqlite3_errmsg(g_db) << std::endl;
                throw ThrowExceptionDefine::SQLITE3_OPEN_V2;
            }

            if (isWal) {
                result = sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &zErrMsg);
                if (result != SQLITE_OK) {
                    std::cout << "[SqliteDB][InitDataBase][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] PRAGMA journal_mode=WAL; failed: " << zErrMsg << std::endl;
                    throw ThrowExceptionDefine::SQLITE3_PRAGMA_JOURNAL_MODE_WAL;
                }
            }

            std::string sqlCmd;
            synchronousLevel == SynchronousLevel::OFF ? sqlCmd = "PRAGMA synchronous = OFF;" : synchronousLevel == SynchronousLevel::NORMAL ? sqlCmd = "PRAGMA synchronous = NORMAL;" : sqlCmd = "PRAGMA synchronous = FULL;";
            result = sqlite3_exec(db, sqlCmd.c_str(), nullptr, nullptr, &zErrMsg);
            if (result != SQLITE_OK) {
                std::cout << "[SqliteDB][InitDataBase][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] " << sqlCmd <<  " failed: " << zErrMsg << std::endl;
                throw ThrowExceptionDefine::SQLITE3_PRAGMA_SYNCHRONOUS_FULL;
            }

            CreateTabel(db);

            sqlite3_close_v2(db);
            return true;
        }
        catch (ThrowExceptionDefine& errorCode) {
            sqlite3_close_v2(db);
            return false;
        }
        catch (...) {
            return false;
        }
    }

    bool SqliteDB::ExecuteBatch(const std::string& querySqlCmd, std::list<Binder>* pBinderList, std::list<Parser>* pParserList)
    {
        try {
            ConnectDB();
            if (pParserList == nullptr) { // 代表DML
                BeginTransaction(CommonSqlite::TransactionLevel::IMMEDIATE);
            } else { // 代表DQL
                BeginTransaction(CommonSqlite::TransactionLevel::DEFERRED);
            }
            if (sqlite3_prepare_v2(g_db, querySqlCmd.c_str(), querySqlCmd.length(), &g_stmt, nullptr) != SQLITE_OK) {
                std::cout << "[SqliteDB][ExecuteBatch][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] sqlite3_prepare_v2 failed: " << sqlite3_errmsg(g_db) << std::endl;
                throw CommonSqlite::ThrowExceptionDefine::SQLITE3_PREPARE_V2;
            }
            if (pBinderList != nullptr) {
                for (auto binder : *pBinderList) {
                    if (binder.HasColumnPair()) {
                        if (sqlite3_reset(g_stmt) != SQLITE_OK) {
                            std::cout << "[SqliteDB][ExecuteBatch][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] sqlite3_reset failed: " << sqlite3_errmsg(g_db) << std::endl;
                            throw CommonSqlite::ThrowExceptionDefine::SQLITE3_RESET;
                        }
                        auto typeOneVec = binder.GetTypeOneDatas();
                        for (auto data : typeOneVec) {
                            int column = sqlite3_bind_parameter_index(g_stmt, data.first.c_str());
                            if (sqlite3_bind_int64(g_stmt, column, data.second) != SQLITE_OK) {
                                std::cout << "[SqliteDB][ExecuteBatch][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] sqlite3_bind_int64 failed: " << sqlite3_errmsg(g_db) << std::endl;
                                throw ThrowExceptionDefine::SQLITE3_BIND;
                            }
                        }
                        auto typeTwoVec = binder.GetTypeTwoDatas();
                        for (auto data : typeTwoVec) {
                            int column = sqlite3_bind_parameter_index(g_stmt, data.first.c_str());
                            if (sqlite3_bind_text(g_stmt, column, data.second.c_str(), data.second.length(), SQLITE_TRANSIENT) != SQLITE_OK) {
                                std::cout << "[SqliteDB][ExecuteBatch][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] sqlite3_bind_text failed: " << sqlite3_errmsg(g_db) << std::endl;
                                throw ThrowExceptionDefine::SQLITE3_BIND;
                            }
                        }
                        if (sqlite3_step(g_stmt) != SQLITE_DONE) {
                            std::cout << "[SqliteDB][ExecuteBatch][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] sqlite3_step failed: " << sqlite3_errmsg(g_db) << std::endl;
                            throw ThrowExceptionDefine::SQLITE3_STEP;
                        }
                    }
                }
            }
            if (pParserList != nullptr) {
                int columnNum = sqlite3_column_count(g_stmt);
                if (columnNum != 0) {
                    while (true) {
                        int stepResult = sqlite3_step(g_stmt);
                        if (stepResult == SQLITE_ROW) {
                            int columnNum_ = columnNum;
                            Parser parse;
                            for (int column = 0; column < columnNum; ++column) {
                                if (sqlite3_column_type(g_stmt, column) == SQLITE_INTEGER) {
                                    parse.InsertColumn(std::make_pair(sqlite3_column_name(g_stmt, column), sqlite3_column_int(g_stmt, column)));
                                }
                                else if (sqlite3_column_type(g_stmt, column) == SQLITE_TEXT) {
                                    parse.InsertColumn(std::make_pair(sqlite3_column_name(g_stmt, column), reinterpret_cast<char*>(const_cast<unsigned char*>(sqlite3_column_text(g_stmt, column)))));
                                }
                            }
                            pParserList->push_back(parse);
                        }
                        else if (stepResult == SQLITE_DONE) {
                            std::cout << "[SqliteDB][ExecuteBatch][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] sqlite3_step finished." << std::endl;
                            break;
                        }
                        else {
                            std::cout << "[SqliteDB][ExecuteBatch][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] sqlite3_step failed: " << sqlite3_errmsg(g_db) << " --> stepResult:" << stepResult << " --> stepResult:" << sqlite3_errcode(g_db) << std::endl;
                            throw CommonSqlite::ThrowExceptionDefine::SQLITE3_STEP;
                        }

                    }
                }
            }
            if (sqlite3_finalize(g_stmt) != SQLITE_OK) {
                std::cout << "[SqliteDB][ExecuteBatch][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] sqlite3_finalize failed: " << sqlite3_errmsg(g_db) << std::endl;
                throw CommonSqlite::ThrowExceptionDefine::SQLITE3_FINALIZE;
            }
            CommitTransaction();
        }
        catch (CommonSqlite::ThrowExceptionDefine& errorCode) {
            CatchHandler(errorCode);
            return false;
        }
        return true;
    }

    bool SqliteDB::ConnectDB()
    {
        if (!g_db) {
            int result = sqlite3_open_v2(m_dataBaseFullPath.c_str(), &g_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, nullptr);
            if (result != SQLITE_OK) {
                std::cout << "[SqliteDB][ConnectDB][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] sqlite3_open_v2 failed: " << sqlite3_errmsg(g_db) << std::endl;
                throw ThrowExceptionDefine::SQLITE3_OPEN_V2;
            }
            result = sqlite3_busy_handler(g_db, BusyCallBack, (void*)g_db);
            if (result != SQLITE_OK) {
                std::cout << "[SqliteDB][ConnectDB][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] sqlite3_busy_handler failed: " << sqlite3_errmsg(g_db) << std::endl;
                throw ThrowExceptionDefine::SQLITE3_BUSY_HANDLER;
            }
            //Common::ExitCallBack func = std::bind(&SqliteDB::DisConnectDB, this);
            //std::vector<Common::ExitCallBack> funcVec{ func };
            Common::OnExitThread(std::bind(&SqliteDB::DisConnectDB, this));
        }

        return true;
    }

    bool SqliteDB::DisConnectDB()
    {
        int result = sqlite3_close_v2(g_db);
        if (result != SQLITE_OK) {
            std::cout << "[SqliteDB][DisConnectDB][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] sqlite3_close_v2 failed: " << sqlite3_errmsg(g_db) << std::endl;
            return false;
        }

        return true;
    }

    bool SqliteDB::BeginTransaction(const TransactionLevel& transactionLevel)
    {
        std::string transactionSql;
        transactionLevel == TransactionLevel::DEFERRED ? transactionSql = "begin;" : transactionLevel == TransactionLevel::IMMEDIATE ? transactionSql = "begin immediate transaction;" : transactionSql = "begin exclusive transaction;";
        char* zErrMsg = nullptr;
        int result = sqlite3_exec(g_db, transactionSql.c_str(), nullptr, nullptr, &zErrMsg);
        if (result != SQLITE_OK) {
            std::cout << "[SqliteDB][BeginTransaction][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] " << transactionSql << " failed: " << zErrMsg << std::endl;
            throw ThrowExceptionDefine::SQLITE3_EXEC_BEGIN;
        }

        return true;
    }

    bool SqliteDB::RollbackTransaction()
    {
        try {
            int result = sqlite3_exec(g_db, "rollback;", nullptr, nullptr, nullptr);
            if (result != SQLITE_OK) {
                throw ThrowExceptionDefine::SQLITE3_EXEC_ROLLBACK;
            }
        }
        catch (ThrowExceptionDefine& errorCode) {
            throw errorCode;
            return false;
        }

        return true;
    }

    bool SqliteDB::CommitTransaction()
    {
        try {
            int result = sqlite3_exec(g_db, "commit;", nullptr, nullptr, nullptr);
            if (result != SQLITE_OK) {
                throw ThrowExceptionDefine::SQLITE3_EXEC_COMMIT;
            }
        }
        catch (ThrowExceptionDefine& errorCode) {
            throw errorCode;
            return false;
        }

        return true;
    }

    void SqliteDB::CatchHandler(ThrowExceptionDefine& errorCode)
    {
        try {
            switch (errorCode) {
            case ThrowExceptionDefine::SQLITE3_COLUMN_xxx:
            case ThrowExceptionDefine::SQLITE3_STEP:
            case ThrowExceptionDefine::SQLITE3_RESET:
                sqlite3_finalize(g_stmt);
            case ThrowExceptionDefine::SQLITE3_FINALIZE:
            case ThrowExceptionDefine::SQLITE3_EXEC_ROLLBACK:
            case ThrowExceptionDefine::SQLITE3_EXEC_COMMIT:
                RollbackTransaction();
            case ThrowExceptionDefine::SQLITE3_EXEC_BEGIN:
            case ThrowExceptionDefine::SQLITE3_PREPARE_V2:
            case ThrowExceptionDefine::SQLITE3_EXEC_CREATE_TABLE:
            case ThrowExceptionDefine::SQLITE3_PRAGMA_SYNCHRONOUS_FULL:
            case ThrowExceptionDefine::SQLITE3_PRAGMA_SYNCHRONOUS_NORMAL:
            case ThrowExceptionDefine::SQLITE3_PRAGMA_SYNCHRONOUS_OFF:
            case ThrowExceptionDefine::SQLITE3_PRAGMA_JOURNAL_MODE_WAL:
            case ThrowExceptionDefine::SQLITE3_BUSY_HANDLER:
                DisConnectDB();
                break;
            default:
                break;
            }
        }
        catch (...) {
            return;
        }

    }

}