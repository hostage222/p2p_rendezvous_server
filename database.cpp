#include "database.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <mutex>
#include <fstream>

#include <boost/filesystem.hpp>

using namespace std;
using namespace boost;

const string DATABASE_FILE_NAME = "database.txt";
const string DATABASE_TEMP_FILE_NAME = "database_temp.txt";

database::database()
{
    load_from_file();
    save_data_thread = thread([this](){ save_data(); });
}

database &database::instance()
{
    static database inst;
    return inst;
}

string database::autorize(const string &phone, const string &password)
{
    lock_guard<mutex> lck{users_mutex};

    auto it = users.find(phone);
    if (it != users.end())
    {
        if (!it->second.autorized)
        {
            if (password == it->second.password)
            {
                it->second.autorized = true;
                return "OK";
            }
            else
            {
                return "INVALID_PASSWORD";
            }
        }
        else
        {
            return "INVALID_ACTION";
        }
    }
    else
    {
        return "INVALID_PHONE";
    }
}

void database::unautorize(const string &phone)
{
    lock_guard<mutex> lck{users_mutex};

    auto it = users.find(phone);
    if (it != users.end())
    {
        it->second.autorized = false;
    }
}

string database::register_(const string &phone, const string &password,
                           const string &code)
{
    (void)code;
    lock_guard<mutex> lck{users_mutex};

    auto it = users.find(phone);
    if (it == users.end())
    {
        users[phone] = {phone, password, false};
        return "OK";
    }
    else
    {
        return "ALREADY_EXISTS";
    }
}

string database::unregister(const string &phone, const string &password)
{
    lock_guard<mutex> lck{users_mutex};

    auto it = users.find(phone);
    if (it != users.end())
    {
        if (!it->second.autorized)
        {
            if (it->second.password == password)
            {
                users.erase(it);
                has_new_data = true;
                new_data_cond_var.notify_one();
                return "OK";
            }
            else
            {
                return "INVALID_PASSWORD";
            }
        }
        else
        {
            return "INVALID_ACTION";
        }
    }
    else
    {
        return "INVALID_PHONE";
    }
}

void database::save_data()
{
    bool stop = false;
    while (!stop)
    {
        unique_lock<mutex> lck{users_mutex};
        new_data_cond_var.wait(lck,
            [this](){ return has_new_data || !save_data_run_flag; });
        if (!save_data_run_flag)
        {
            stop = true;
        }

        save_to_file();
    }
}

void database::save_to_file()
{
    users_mutex.lock();
    if (!has_new_data)
    {
        return;
    }
    auto saving_users = users;
    users_mutex.unlock();

    try
    {
        filesystem::remove(DATABASE_TEMP_FILE_NAME);
    }
    catch (filesystem::filesystem_error&)
    {
        cout << "Fatal error: couldn't remove file \"" <<
                DATABASE_TEMP_FILE_NAME << "\"" << endl;
        exit(-1);
    }

    ofstream file{DATABASE_TEMP_FILE_NAME};
    for (const auto &u : saving_users)
    {
        save_param(file, u.second.phone);
        file << " ";
        save_param(file, u.second.password);
        file << "\n";
    }
    file.close();
    if (file.fail())
    {
        cout << "Fatal error: couldn't save data to file \"" <<
                DATABASE_TEMP_FILE_NAME << "\"" << endl;
    }

    try
    {
        filesystem::remove(DATABASE_FILE_NAME);
    }
    catch (filesystem::filesystem_error&)
    {
        cout << "Fatal error: couldn't remove file \"" <<
                DATABASE_FILE_NAME << "\"" << endl;
        exit(-1);
    }

    try
    {
        filesystem::rename(DATABASE_TEMP_FILE_NAME, DATABASE_FILE_NAME);
    }
    catch (filesystem::filesystem_error&)
    {
        cout << "Fatal error: couldn't rename file from \"" <<
                DATABASE_TEMP_FILE_NAME << "\" to \"" <<
                DATABASE_FILE_NAME << "\"" << endl;
        exit(-1);
    }
}

void database::load_from_file()
{
    string fname;

    if (filesystem::exists(DATABASE_FILE_NAME))
    {
        fname = DATABASE_FILE_NAME;
    }
    else if (filesystem::exists(DATABASE_TEMP_FILE_NAME))
    {
        fname = DATABASE_TEMP_FILE_NAME;
    }
    else
    {
        return;
    }

    try
    {
        ifstream file{fname};
        while (true)
        {
            char c;
            do
            {
                c = read_symbol(file);
            }
            while (c == '\r' || c == '\n');
            if (file.eof())
            {
                break;
            }

            if (c != '"')
            {
                throw invalid_format_exception{};
            }

            string phone = load_param(file);
            if (read_symbol(file) != ' ' ||
                read_symbol(file) != '"')
            {
                throw invalid_format_exception{};
            }
            string password = load_param(file);

            users[phone] = user_data{phone, password, false};
        }
    }
    catch (invalid_format_exception&)
    {
        users.clear();
    }
    catch (read_exception&)
    {
        users.clear();
    }
}

void database::save_param(ofstream &file, const string &param)
{
    file << "\"";

    for (char c : param)
    {
        if (c == '"')
        {
            file << "\\\"";
        }
        else
        {
            file << c;
        }
    }

    file << "\"";
}

string database::load_param(ifstream &file)
{
    bool spec_symbol = false;
    string res;

    while (true)
    {
        char c = read_symbol(file);
        if (file.eof())
        {
            throw invalid_format_exception{};
        }

        if (spec_symbol)
        {
            if (c == '\\')
            {
                res.push_back('\\');
                spec_symbol = false;
            }
            else if (c == '"')
            {
                res.push_back('"');
                spec_symbol = false;
            }
            else
            {
                throw invalid_format_exception{};
            }
        }
        else
        {
            if (c == '"')
            {
                return res;
            }
            else if (c == '\\')
            {
                spec_symbol = true;
            }
            else
            {
                res.push_back(c);
            }
        }
    }
}

int database::read_symbol(ifstream &file)
{
    int res = file.get();
    if (file.fail() && !file.eof())
    {
        throw read_exception{};
    }
}
