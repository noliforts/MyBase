#include <gtest/gtest.h>
#include "db_core/database_manager.h"
#include <thread>
#include <vector>

TEST(ConcurrencyTest, ConcurrentInsertsAreThreadSafe) {
    const int N_THREADS   = 10;
    const int ROWS_EACH   = 50;

    DatabaseManager& mgr = DatabaseManager::instance();
    {
        Session s;
        mgr.createDatabase("concdb", s);
        mgr.useDatabase("concdb", s);
        mgr.getCurrentDatabase(s).createTable("nums", TableSchema{{{"v", DataType::INT}}});
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < N_THREADS; ++i) {
        threads.emplace_back([&mgr, i]() {
            Session s;
            s.currentDb = "concdb";
            for (int j = 0; j < ROWS_EACH; ++j) {
                std::lock_guard<std::mutex> lock(mgr.mutex());
                mgr.getCurrentDatabase(s).getTable("nums").insert({i * ROWS_EACH + j});
            }
        });
    }
    for (auto& t : threads) t.join();

    Session s;
    s.currentDb = "concdb";
    std::lock_guard<std::mutex> lock(mgr.mutex());
    auto rows = mgr.getCurrentDatabase(s).getTable("nums").select({"*"}, nullptr);
    EXPECT_EQ(rows.size(), static_cast<size_t>(N_THREADS * ROWS_EACH));
}

TEST(ConcurrencyTest, IsolatedSessionsDoNotInterfere) {
    DatabaseManager& mgr = DatabaseManager::instance();
    {
        Session s;
        mgr.createDatabase("dbA", s);
        mgr.createDatabase("dbB", s);
        mgr.useDatabase("dbA", s);
        mgr.getCurrentDatabase(s).createTable("data", TableSchema{{{"x", DataType::INT}}});
        mgr.useDatabase("dbB", s);
        mgr.getCurrentDatabase(s).createTable("data", TableSchema{{{"x", DataType::INT}}});
    }
    
    std::thread t1([&mgr]() {
        Session s;
        s.currentDb = "dbA";
        for (int i = 0; i < 20; ++i) {
            std::lock_guard<std::mutex> lock(mgr.mutex());
            mgr.getCurrentDatabase(s).getTable("data").insert({i});
        }
    });
    std::thread t2([&mgr]() {
        Session s;
        s.currentDb = "dbB";
        for (int i = 0; i < 30; ++i) {
            std::lock_guard<std::mutex> lock(mgr.mutex());
            mgr.getCurrentDatabase(s).getTable("data").insert({i});
        }
    });
    t1.join();
    t2.join();

    Session sA; sA.currentDb = "dbA";
    Session sB; sB.currentDb = "dbB";
    std::lock_guard<std::mutex> lock(mgr.mutex());
    EXPECT_EQ(mgr.getCurrentDatabase(sA).getTable("data").select({"*"}, nullptr).size(), 20u);
    EXPECT_EQ(mgr.getCurrentDatabase(sB).getTable("data").select({"*"}, nullptr).size(), 30u);
}