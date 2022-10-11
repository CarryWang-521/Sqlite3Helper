#include "FastAccessDBModule.h"
#include <iostream>
#include <list>

using namespace CommonSqlite;
namespace FastAccessDB {
    FastAccessDBModule& FastAccessDBModule::GetInstance()
    {
        static FastAccessDBModule instance;
        return instance;
    }
    bool FastAccessDBModule::InsertDatas(const std::vector<FastAccessData>& fastAccessDatas)
    {
        std::string insertSqlCmd = "insert into testTable(nId,nIndex,fileId) values(:nId,:nIndex,:fileId);";
        // 构造数据:将fastAccessDatas 转换为 pBinderList
        int num = 1000;
        std::string fileId = "fileId_";
        std::list<Binder>* pBinderList = new std::list<Binder>;
        for (int i = 0; i < num; ++i) {
            Binder binder;
            binder.InsertColumn(std::make_pair(":nIndex", i));
            binder.InsertColumn(std::make_pair(":fileId", fileId + std::to_string(i)));
            pBinderList->push_back(binder);
        }

        ExecuteBatch(insertSqlCmd, pBinderList, nullptr);
        return false;
    }
    std::vector<FastAccessData>* FastAccessDBModule::QueryAllFastAccessDatas()
    {
        std::string insertSqlCmd = "select * from testTable;";
        std::list<Parser>* pParserList = new std::list<Parser>;
        ExecuteBatch(insertSqlCmd, nullptr, pParserList);

        std::vector<FastAccessData>* pFastAccessDataVec = new std::vector<FastAccessData>;
        for (auto parser : *pParserList) {
            FastAccessData fastAccessData;
            if (parser.HasColumnPair()) {
                auto typeOneVec = parser.GetTypeOneDatas();
                for (auto data : typeOneVec) {
                    if (data.first.find("nId") != std::string::npos) {
                        fastAccessData.nId = data.second;
                    }
                    else if (data.first.find("nIndex") != std::string::npos) {
                        fastAccessData.nIndex = data.second;
                    }
                }
                auto typeTwoVec = parser.GetTypeTwoDatas();
                for (auto data : typeTwoVec) {
                    if (data.first.find("fileId") != std::string::npos) {
                        fastAccessData.fileId = data.second;
                    }
                }
                pFastAccessDataVec->push_back(fastAccessData);
            }
        }
        return pFastAccessDataVec;
    }
    bool FastAccessDBModule::ParseQueryAllFastAccessDatas(sqlite3_stmt* stmt, void* pExtern)
    {
        try {
            std::list<FastAccessData*>* pDataList = static_cast<std::list<FastAccessData*>*>(pExtern);

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                FastAccessData* pdata = new FastAccessData();
                pdata->nId = sqlite3_column_int(stmt, 0);
                pdata->nIndex = sqlite3_column_int(stmt, 1);
                pdata->fileId = std::string((char*)sqlite3_column_text(stmt, 2));

                pDataList->push_back(pdata);
                std::cout << *pdata << std::endl;
            }
        }
        catch (...) {
            std::cout << "[SqliteDB][ParseQueryAllFastAccessDatas][" << _getpid() << "][" << std::this_thread::get_id() << "][g_db:" << std::hex << g_db << "] sqlite3_step 0r sqlite3_column_*** failed!!!" << std::endl;
            return false;
        }

        return true;
    }
    bool FastAccessDBModule::CreateTabel(sqlite3* db)
    {
        if (!db) {
            return false;
        }
        try {
            char* zErrMsg = nullptr;
            int result = sqlite3_exec(db, "create table if not exists testTable(nId integer primary key autoincrement, nIndex int not null, fileId TEXT)", nullptr, nullptr, &zErrMsg);
            if (result != SQLITE_OK) {
                std::cout << "[SqliteDB][CreateTabel][" << _getpid() << "][" << std::this_thread::get_id() << "][db:" << std::hex << db << "] failed: " << zErrMsg << std::endl;
                throw CommonSqlite::ThrowExceptionDefine::SQLITE3_EXEC_CREATE_TABLE;
            }
        }
        catch (CommonSqlite::ThrowExceptionDefine& errorCode) {
            CatchHandler(errorCode);
            return false;
        }

        return true;
    }
}