#ifndef FAST_ACCESS_DB_MODULE_H
#define FAST_ACCESS_DB_MODULE_H
#include <map>
#include <mutex>
#include <iostream>
#include <iomanip>
#include "SqliteDB.h"

#define FASTACCESS_DB_MODULE FastAccessDB::FastAccessDBModule::GetInstance()
#define FAST_ACCESS_DB_NS FastAccessDB::FastAccessDBModule

namespace FastAccessDB {

    struct FastAccessData {
        int nId;
        int nIndex;
        std::string fileId;
        FastAccessData() = default;
        FastAccessData(const int& id, const int& index, const std::string& fileId) : nId(id), nIndex(index), fileId(fileId) {};

        friend std::ostream& operator<<(std::ostream& cout, const FastAccessData& fastAccessData);
    };
    // 添加inlie，避免重定义
    inline std::ostream& operator<<(std::ostream& os, const FastAccessData& fastAccessData)
    {
        return os << std::setiosflags(std::ios::left) << "id: " << fastAccessData.nId << "\t" << "index: " << fastAccessData.nIndex << "\t" << "fileId: " << fastAccessData.fileId;
    }

    class FastAccessDBModule : public CommonSqlite::SqliteDB
    {
    public:
        static FastAccessDBModule& GetInstance();
        ~FastAccessDBModule() = default;
    public:
        bool InsertDatas(const std::vector<FastAccessData>& fastAccessDatas);
        std::vector<FastAccessData>* QueryAllFastAccessDatas();

    public:
        static bool ParseQueryAllFastAccessDatas(sqlite3_stmt* stmt, void* pExtern);

    private:
        virtual bool CreateTabel(sqlite3* db) override;
    private:
        FastAccessDBModule() = default;
    private:
        std::mutex m_mutex;
        std::atomic<int> m_id = 0;
    };
}

#endif


