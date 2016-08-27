#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <math.h>
#include <assert.h>
#include "eclat.h"

namespace eclat {

using namespace std;

// 比较前缀
bool ComparePrefix(const Itemset &lhs, const Itemset &rhs)
{
    for(size_t i = 0; i < lhs.size()-1; ++i)
    {
        if (lhs[i] != rhs[i])
            return false;
    }
    return true;
}

// 合成多一项的itemset
Itemset GenerateLongerItemset(const Itemset &lhs, const Itemset &rhs)
{
    assert(lhs.size() == rhs.size());
    if (*(lhs.rbegin()) <= *(rhs.rbegin()))
    {
        Itemset vec(lhs);
        vec.reserve(lhs.size()+1);
        vec.push_back(*(rhs.rbegin()));
        return vec;
    }
    Itemset vec(rhs);
    vec.reserve(rhs.size()+1);
    vec.push_back(*(lhs.rbegin())); //make sure sorted
    return vec;
}

///////////////////////////////
template<typename T>
void EclatImpl::Intersect(const std::vector<T> &vec1, const std::vector<T> &vec2, std::vector<T> &result_vec)
{
    result_vec.clear();
    size_t i = 0;
    size_t j = 0;
    while (i < vec1.size() && j < vec2.size())
    {
        if(vec1[i] == vec2[j])
        {
            result_vec.push_back(vec1[i]);
            i++;
            j++;
        }
        else if(vec1[i] < vec2[j])
            i++;
        else
            j++;
    }
}

void EclatImpl::GenerateOneItemset()
{
    if (m_finish || m_invert_vec.size() >= 1)
        return;
    m_one_item_freq.clear();
    m_one_item_tids.clear();
    for (size_t i = 0; i < m_transactions.size(); ++i)
    {
        const vector<WordID> &ids = m_transactions[i]; 
        for(size_t j = 0; j < ids.size(); ++j)
        {
            WordID id = ids[j];
            if (m_one_item_freq.find(id) == m_one_item_freq.end())
                m_one_item_freq.insert(make_pair(id, 1));
            else
                m_one_item_freq[id]++;

            if (m_one_item_tids.find(id) != m_one_item_tids.end())
                m_one_item_tids[id].push_back(i);
            else
                m_one_item_tids.insert(make_pair(id, vector<TransID>(1,i)));
        }
    }
    map<WordID, size_t>::iterator oiter = m_one_item_freq.begin();
    while (oiter != m_one_item_freq.end())
    {
        if (oiter->second < m_support)
        {
            m_one_item_tids.erase(oiter->first); 
            m_one_item_freq.erase(oiter++); //注意map删除元素
        }
        else
            oiter++;
    }
    if (m_one_item_tids.empty())
    {
        m_finish = true;
        return;
    }
    m_invert_vec.push_back(map<Itemset, vector<TransID> >());
    map<Itemset, vector<TransID> > &item_tids = m_invert_vec.back();
    map<WordID, vector<TransID> >::iterator iter = m_one_item_tids.begin();
    for (; iter != m_one_item_tids.end(); ++iter)
    {
        vector<WordID> key(1, iter->first);
        item_tids.insert(make_pair(key, iter->second));
    }
}

void EclatImpl::GenerateTwoItemset()
{
    if (m_finish || m_invert_vec.size() >= 2)
        return;
    if (m_invert_vec.size() < 1)
        GenerateOneItemset();

    m_invert_vec.push_back(map<Itemset, vector<TransID> >());
    map<Itemset, vector<TransID> > &two_item_tids = m_invert_vec.back();

    two_item_tids.clear();
    vector<TransID> result_vec;
    map<WordID, vector<TransID> >::iterator iter = m_one_item_tids.begin();
    //cout << "m_one_item_tids size: " << m_one_item_tids.size() << endl;
    for (; iter != m_one_item_tids.end(); ++iter)
    {
        map<WordID, vector<TransID> >::iterator other_iter = iter; // TransID is sorted
        ++other_iter;
        for (; other_iter != m_one_item_tids.end(); ++other_iter) 
        {
            Intersect(iter->second, other_iter->second, result_vec);
            if (result_vec.size() < m_support)
                continue;
            Bigram bi(2,0);
            bi.reserve(2);
            bi[0] = iter->first;
            bi[1] = other_iter->first;
            two_item_tids.insert(make_pair(bi, result_vec));
        }
    }
    if (two_item_tids.empty())
    {
        m_invert_vec.pop_back();
        m_finish = true;
    }
}

void EclatImpl::GenerateNextItemset(const map<Itemset, vector<TransID> > &item_tids, map<Itemset, vector<TransID> > &new_item_tids)
{
    if (m_finish)
        return;
    vector<TransID> result_vec;
    map<Itemset, vector<TransID> >::const_iterator iter = item_tids.begin();
    map<Itemset, vector<TransID> >::const_iterator other_iter;
    //cout << "item_tids size: " << item_tids.size() << endl;
    for (; iter != item_tids.end(); ++iter)
    {
        other_iter = iter;
        ++other_iter;
        for (; other_iter != item_tids.end(); ++other_iter) 
        {
            if (!ComparePrefix(iter->first, other_iter->first)) // the WordID in itemset is sorted
                break;
            Intersect(iter->second, other_iter->second, result_vec);
            if (result_vec.size() < m_support)
                continue;
            new_item_tids.insert(make_pair(GenerateLongerItemset(iter->first, other_iter->first), result_vec));
        }
    }
}

void EclatImpl::GenerateItemset(size_t k)
{
    if (k < 1)
        return;
    if (k == 1)
    {
        GenerateOneItemset();
        return;
    }
    if (m_invert_vec.size() < 2)
        GenerateTwoItemset();
    if (m_finish)
        return;
    for (size_t i = m_invert_vec.size(); i < k; ++i)
    {
        m_invert_vec.push_back(map<Itemset, vector<TransID> >());
        map<Itemset, vector<TransID> > &new_map = m_invert_vec.back();
        GenerateNextItemset(m_invert_vec[i-1], new_map);
        if (new_map.empty())
        {
            m_finish = true;
            m_invert_vec.pop_back();
            break;
        }
    }
}

void EclatImpl::GenerateAllItemset()
{
    size_t k = 1;
    while (true)
    {
        GenerateItemset(k);
        if (m_finish)
            break;
        cout << "frequent " << k << "-itemset size:\t" << m_invert_vec[k-1].size() << endl;
        k++;
    }
}

void EclatImpl::ShowItemset(size_t start, size_t end) const
{
    if (start < 1 || start > end || end > m_invert_vec.size())
        return;
    for (size_t i = start; i <= end; ++i)
    {
        const map<Itemset, vector<TransID> > &item_tids = m_invert_vec[i-1];
        map<Itemset, vector<TransID> >::const_iterator iter = item_tids.begin();
        for (; iter != item_tids.end(); ++iter)
            cout << iter->first.size() << '\t' << iter->first << '\t' << iter->second.size() << endl;
    }
}

void EclatImpl::ShowItemset(size_t start, size_t end, const std::vector<std::string> &word_vec) const
{
    if (start < 1 || start > end || end > m_invert_vec.size())
        return;
    for (size_t i = start; i <= end; ++i)
    {
        const map<Itemset, vector<TransID> > &item_tids = m_invert_vec[i-1];
        map<Itemset, vector<TransID> >::const_iterator iter = item_tids.begin();
        for (; iter != item_tids.end(); ++iter)
        {
            const Itemset &x = iter->first;
            cout << x.size() << '\t' << word_vec[x[0]];
            for (size_t j = 1; j < x.size(); ++j)
                cout << ' ' << word_vec[x[j]];
            cout << '\t' << iter->second.size() << endl;
        }
    }
}


void EclatImpl::CalcMutualInfo(map<Bigram, double> &mi_map)
{
    mi_map.clear();
    if (m_invert_vec.size() < 2)
        GenerateTwoItemset();
    map<Bigram, vector<TransID> > &two_item_tids = m_invert_vec[1];
    map<Bigram, vector<TransID> >::const_iterator iter = two_item_tids.begin();
    size_t total = m_transactions.size();
    for (; iter != two_item_tids.end(); ++iter)
    {
        size_t joint_freq = iter->second.size();
        const Bigram &bi = iter->first;
        size_t n1 = m_one_item_freq[bi[0]];
        size_t n2 = m_one_item_freq[bi[1]];
        double mi = log2(double(joint_freq)*total/(n1*n2));
        mi_map.insert(make_pair(bi, mi));
    }
}

template <typename T>
bool EclatImpl::IsSorted(const vector<T> &vec)
{
    if (vec.size() < 2)
        return true;
    for(size_t i = 1; i < vec.size(); ++i)
    {
        if (vec[i-1] > vec[i])
            return false;
    }
    return true;
}

} // namespace eclat
