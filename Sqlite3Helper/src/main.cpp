#include <iostream>
#include <thread>
#include "Sqlite3Helper/FastAccessDBModule.h"

int main()
{
    FASTACCESS_DB_MODULE.InitDataBase("FastAccessDB.dat");

    for (int i = 0; i < 10; i++) {
        std::thread([]() {
            std::vector<FastAccessDB::FastAccessData> fastAccessDatas;
            FASTACCESS_DB_MODULE.InsertDatas(fastAccessDatas);
        }).detach();
    }

    std::thread([]() {
        while (true) {
            std::vector<FastAccessDB::FastAccessData>* pDataVec = FASTACCESS_DB_MODULE.QueryAllFastAccessDatas();
            std::this_thread::sleep_for(std::chrono::seconds(10));
            std::cout << "*************************************************************************************" << std::endl;
        }
    }).detach();


    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}