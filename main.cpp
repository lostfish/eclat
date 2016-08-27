#include<iostream>
#include<fstream>
#include<algorithm>
#include<set>
#include<cstdlib>
#include "eclat.h"

using namespace std;
using namespace eclat;

#define _USE_WORD_INDEX_

#ifdef _USE_WORD_INDEX_
//vector<string> g_record_vec;
vector<std::string> g_word_vec;
map<std::string, WordID> g_word_index;
#endif

void split(const string &s, const string &delim, vector<string> &elems, size_t max_split = 0)
{
    elems.clear();
    if (s.empty())
        return;
    string::size_type next = string::npos;
    string::size_type current = 0;
    size_t len = delim.size();
    while (true) {
        next = s.find(delim, current);
        elems.push_back(s.substr(current, next - current));
        if (next == string::npos)
            break;
        current = next+len;
        if (max_split > 0 && max_split == elems.size()) {
            elems.push_back(s.substr(current));
            break;
        }  
    }  
}

bool read_file(const string &input_file, EclatImpl &eclat)
{
    ifstream fin(input_file.c_str());
    if (!fin.is_open())
    {
        cout << "fail to open file: " << input_file << endl;
        return false;
    }
    string line;
    vector<string> fields;
    set<WordID> occur_set;
    set<WordID> uniq_id_set;
    size_t total_num = 0;
    size_t valid_num = 0;
    while (getline(fin, line))
    {
        total_num++;
        split(line, " ", fields);
        occur_set.clear();
        vector<WordID> id_vec;
        id_vec.reserve(fields.size());
        for (size_t i = 0; i < fields.size(); ++i)
        {
#ifdef _USE_WORD_INDEX_
            string &word = fields[i];
            if (g_word_index.find(word) == g_word_index.end())
            {
                g_word_vec.push_back(word);
                g_word_index.insert(make_pair(word, g_word_vec.size()-1));
            }
            WordID id = g_word_index.at(word); 
#else
            WordID id = atoi(fields[i].c_str());
#endif
            if (occur_set.find(id) != occur_set.end())
                continue;
            uniq_id_set.insert(id);
            occur_set.insert(id);
            id_vec.push_back(id);
        }
        sort(id_vec.begin(), id_vec.end()); //from less to great
        if (id_vec.empty())
            continue;
        valid_num++;
        //g_record_vec.push_back(line);
        eclat.AddTransaction(id_vec); // 插入WordID去重后的记录
    }
    fin.close();
    cout << "valid_num: " << valid_num << '\t' << total_num << endl;
    cout << "uniq id num: " << uniq_id_set.size() << endl;
    //set<WordID>::iterator iter = uniq_id_set.begin();
    //for (; iter != uniq_id_set.end(); ++iter)
    //    cout << *iter << endl;
    return true;

}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "Usage: " << argv[0] << " <infile> <support>" << endl;
        return -1;
    }
    string input_file(argv[1]);
    int support = atoi(argv[2]);

    EclatImpl eclat(support);
    read_file(input_file, eclat);

    eclat.GenerateAllItemset();
    size_t max_k = eclat.GetMaxItemNum();
#ifdef _USE_WORD_INDEX_
    eclat.ShowItemset(1, max_k, g_word_vec);
#else
    eclat.ShowItemset(1, max_k);
#endif
    return 0;
}
