#include "resmanage/ResManager.hpp"
#include <atomic>
#include <gtest/gtest.h>
#include <thread>

namespace fs = std::filesystem;

class ResManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        testBasePath_ = "./test_res/";
        res::ResManager::get_instance();
    }

    void TearDown() override
    {
        if (fs::exists(testBasePath_))
        {
            fs::remove_all(testBasePath_);
        }
    }

    std::string testBasePath_ = "./test_res/";
};

TEST_F(ResManagerTest, SingletonPattern)
{
    auto& instance1 = res::ResManager::get_instance();
    auto& instance2 = res::ResManager::get_instance();
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(ResManagerTest, GetBasePath)
{
    auto& manager = res::ResManager::get_instance();
    EXPECT_EQ(manager.get_base_path(), "res/");
}

TEST_F(ResManagerTest, GetResourcePath)
{
    auto& manager = res::ResManager::get_instance();
    EXPECT_EQ(manager.get_res_path(res::ResManager::ResourceType::Texture), "res/Texture/");
    EXPECT_EQ(manager.get_res_path(res::ResManager::ResourceType::Model), "res/Model/");
    EXPECT_EQ(manager.get_res_path(res::ResManager::ResourceType::Audio), "res/Audio/");
}

TEST_F(ResManagerTest, ResourceExists)
{
    auto& manager = res::ResManager::get_instance();
    EXPECT_FALSE(manager.resource_exists(res::ResManager::ResourceType::Texture, "nonexistent.txt"));
}

TEST_F(ResManagerTest, WriteAndReadResource)
{
    auto& manager = res::ResManager::get_instance();
    fs::path testDir = "./res/Config/";
    fs::create_directories(testDir);
    fs::path testFile = testDir / "test.json";
    std::string content = "{\"key\": \"value\"}";

    {
        std::ofstream out(testFile);
        out << content;
    }

    auto readContent = manager.read_resource_text(res::ResManager::ResourceType::Config, "test.json");
    ASSERT_TRUE(readContent.has_value());
    EXPECT_EQ(readContent.value(), content);
}

TEST_F(ResManagerTest, ReadNonexistentResource)
{
    auto& manager = res::ResManager::get_instance();
    auto result = manager.read_resource_text(res::ResManager::ResourceType::Texture, "nonexistent.txt");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ResManagerTest, ListResources)
{
    auto& manager = res::ResManager::get_instance();
    fs::path testDir = "./res/Texture/";
    fs::create_directories(testDir);

    {
        std::ofstream(testDir / "file1.png") << "data1";
        std::ofstream(testDir / "file2.jpg") << "data2";
        std::ofstream(testDir / "file3.bmp") << "data3";
    }

    auto files = manager.list_resources(res::ResManager::ResourceType::Texture);
    EXPECT_EQ(files.size(), 3);
    EXPECT_TRUE(std::find(files.begin(), files.end(), "file1.png") != files.end());
    EXPECT_TRUE(std::find(files.begin(), files.end(), "file2.jpg") != files.end());
    EXPECT_TRUE(std::find(files.begin(), files.end(), "file3.bmp") != files.end());
}

TEST_F(ResManagerTest, GetResourcePathFull)
{
    auto& manager = res::ResManager::get_instance();
    fs::path testDir = "res/Texture/";
    fs::create_directories(testDir);
    fs::path testFile = testDir / "test.png";
    std::ofstream out(testFile);
    out << "fake png data";

    auto path = manager.get_resource_path(res::ResManager::ResourceType::Texture, "test.png");
    EXPECT_TRUE(path.has_value());
    EXPECT_EQ(path.value(), "res/Texture/test.png");
}

TEST_F(ResManagerTest, ThreadSafety)
{
    auto& manager = res::ResManager::get_instance();
    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);

    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back(
            [&manager, &successCount, i]()
            {
                auto basePath = manager.get_base_path();
                auto resPath = manager.get_res_path(res::ResManager::ResourceType::Texture);
                auto exists = manager.resource_exists(res::ResManager::ResourceType::Texture, "test.txt");
                if (!basePath.empty() && !resPath.empty())
                {
                    successCount++;
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(successCount, 10);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
