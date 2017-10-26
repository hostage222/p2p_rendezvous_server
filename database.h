#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <fstream>

class database
{
public:
    static database& instance();

    std::string autorize(const std::string &phone,
                         const std::string &password);
    void unautorize(const std::string &phone);
    std::string register_(const std::string &phone,
                          const std::string &password,
                          const std::string &code);
    std::string unregister(const std::string &phone,
                           const std::string &password);

private:
    database();

    struct user_data
    {
        std::string phone;
        std::string password;
        bool autorized;
    };
    std::unordered_map<std::string, user_data> users;
    std::mutex users_mutex;
    bool has_new_data = false;
    std::condition_variable new_data_cond_var;

    std::atomic<bool> save_data_run_flag{true};
    std::thread save_data_thread;
    void save_data();

    void save_to_file();
    void load_from_file();
    void save_param(std::ofstream &file, const std::string &param);
    struct invalid_format_exception{};
    struct read_exception{};
    std::string load_param(std::ifstream &file);
    int read_symbol(std::ifstream &file);
};

#endif // DATABASE_H
