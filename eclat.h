#ifndef ECLAT_H
#define ECLAT_H

#include <iostream>
#include <vector>
#include <map>

namespace eclat {

typedef unsigned int WordID;
typedef size_t TransID;
typedef unsigned char uint8_t;

typedef std::vector<WordID> Itemset;
typedef Itemset Bigram;
typedef Itemset Trigram;

// 比较前缀
bool ComparePrefix(const Itemset &lhs, const Itemset &rhs);

// 合成多一项的itemset
Itemset GenerateLongerItemset(const Itemset &lhs, const Itemset &rhs);

// 重载输出操作符
template<typename T>
std::ostream& operator<< (std::ostream& os, const std::vector<T>& vec)
{
    typename std::vector<T>::const_iterator iter = vec.begin();
    if (iter != vec.end())
    {
        os << *iter;
        for (++iter; iter != vec.end(); ++iter)
            os << ' ' << *iter;
    }
    return os;
}


///////////////////////////////
class EclatImpl //操作的都是WordID,而不是具体的WORD
{
public:
    EclatImpl():m_support(3), m_finish(false) {}
    EclatImpl(unsigned int support):m_support(support), m_finish(false) {}
    ~EclatImpl() {};

    // @brief 清空
    void Clear()
    {
        m_support = 3; //default value
        m_finish = false;
        m_transactions.clear();
        m_one_item_freq.clear();
        m_one_item_tids.clear();
        m_invert_vec.clear();
    }
    
    // @brief 添加记录,每个记录由多个WordID组成
    void AddTransaction(const std::vector<WordID> &trans)
    {
        m_transactions.push_back(trans);
    }

    // @brief 产生频繁k项集
    void GenerateItemset(size_t k);

    // @brief 产生所有频繁项集
    void GenerateAllItemset();

    // @brief 输出频繁项集
    void ShowItemset(size_t start, size_t end) const;

    // @brief 输出频繁项集
    void ShowItemset(size_t start, size_t end, const std::vector<std::string> &word_vec) const;

    // @brief 获取已计算的最大项集数
    size_t GetMaxItemNum() { return m_invert_vec.size(); }

private:
    // @brief 产生频繁一项集
    void GenerateOneItemset();

    // @brief 产生频繁二项集
    void GenerateTwoItemset();

    // @brief 根据频繁k项集产生频繁k+1项集
    void GenerateNextItemset(const std::map<Itemset, std::vector<TransID> > &item_tids, std::map<Itemset, std::vector<TransID> > &new_item_tids);

    // @brief 计算二项集的互信息
    void CalcMutualInfo(std::map<Bigram, double> &mi_map);

    // @brief 两个有序列表求交
    template<typename T>
    void Intersect(const std::vector<T> &vec1, const std::vector<T> &vec2, std::vector<T> &result_vec);

    // @brief 判断列表是否是有序的
    template <typename T>
    bool IsSorted(const std::vector<T> &vec);

private:
    std::vector<std::vector<WordID> > m_transactions;
    unsigned int m_support;
    bool m_finish;
    std::map<WordID, size_t> m_one_item_freq;
    std::map<WordID, std::vector<TransID> > m_one_item_tids;               // 一项集倒排索引
    std::vector< std::map<Itemset, std::vector<TransID> > > m_invert_vec;   // 项集倒排索引(from 2 to n)
};

} // namespace eclat

#endif
