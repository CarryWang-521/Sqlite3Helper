#ifndef RAW_BINDER_H
#define RAW_BINDER_H
#include <vector>

namespace CommonSqlite {

    enum class DataType : int {
        INT,
        STRING
    };

    template <class T1, class T2> // T1和T2可以用于存储int_64和std::wstring相关类型
    class RawBinderBase
    {
    public:
        bool HasColumnPair() {
            return (m_datasOne.size() + m_datasTwo.size()) != 0;
        }
        void InsertColumn(T1 data) {
            m_datasOne.push_back(data);
        }
        void InsertColumn(T2 data) {
            m_datasTwo.push_back(data);
        }
        std::vector<T1> GetTypeOneDatas() {
            return m_datasOne;
        }
        std::vector<T2> GetTypeTwoDatas() {
            return m_datasTwo;
        }
    private:
        //std::vector<DataType> m_dataFlags;
        //int m_dataFlagIndex = 0;
        std::vector<T1> m_datasOne;
        //int m_datasOneIndex = 0;
        std::vector<T2> m_datasTwo;
        //int m_datasTwoIndex = 0;
    };
}

#endif

