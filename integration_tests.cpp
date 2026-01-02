#include <gtest/gtest.h>

#include <cstdio>
#include <ctime>
#include <map>
#include <regex>
#include <string>
#include <vector>

// 组件A：日期
static std::string GetCurrentDate() {
    std::time_t now = std::time(nullptr);
    std::tm ltm{};
#if defined(_WIN32)
    localtime_s(&ltm, &now);
#else
    localtime_r(&now, &ltm);
#endif
    char buf[20];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                  1900 + ltm.tm_year, 1 + ltm.tm_mon, ltm.tm_mday);
    return std::string(buf);
}

// 组件B：分类识别
struct Category {
    int id;
    std::string name;
    std::string description;
};

class CategoryRecognizer {
 public:
    explicit CategoryRecognizer(const std::vector<Category>& categories)
        : categories_(categories) {}

    int RecognizeCategory(const std::string& note) {
        std::map<std::string, int> keyword_map;
        for (const auto& c : categories_) keyword_map[c.name] = c.id;

        for (const auto& kv : keyword_map) {
            if (note.find(kv.first) != std::string::npos) return kv.second;
        }

        for (const auto& c : categories_) {
            if (c.name == "其他") return c.id;
        }
        return categories_.empty() ? 0 : categories_[0].id;
    }

 private:
    std::vector<Category> categories_;
};

// 集成流程：模拟“交易处理”
struct ProcessedTransaction {
    std::string date;
    int category_id;
    std::string note;
};

static ProcessedTransaction ProcessTransaction(const std::string& note,
                                               const std::string& date_input,
                                               const std::vector<Category>& cats) {
    ProcessedTransaction out{};
    out.note = note;
    out.date = date_input.empty() ? GetCurrentDate() : date_input;

    CategoryRecognizer cr(cats);
    out.category_id = cr.RecognizeCategory(note);
    return out;
}

static std::vector<Category> DefaultCats() {
    return {
        {1, "餐饮", "饮食相关"},
        {2, "娱乐", "娱乐消费"},
        {3, "水电费", "生活缴费"},
        {4, "工资", "收入"},
        {5, "其他", "其他"},
    };
}

// ===================== 集成测试：组1（正常流程组合） =====================
TEST(Integration_Group1_NormalFlow, AutoFillDateAndRecognizeCategory) {
    auto out = ProcessTransaction("餐饮 午饭", "", DefaultCats());
    EXPECT_TRUE(std::regex_match(out.date, std::regex(R"(^\d{4}-\d{2}-\d{2}$)")));
    EXPECT_EQ(out.category_id, 1);
}

TEST(Integration_Group1_NormalFlow, KeepManualDateAndRecognizeCategory) {
    auto out = ProcessTransaction("工资 发放", "2026-01-01", DefaultCats());
    EXPECT_EQ(out.date, "2026-01-01");
    EXPECT_EQ(out.category_id, 4);
}

// ===================== 集成测试：组2（边界/回退组合） =====================
TEST(Integration_Group2_Fallbacks, NoKeywordFallsBackToOther) {
    auto out = ProcessTransaction("买书", "", DefaultCats());
    EXPECT_TRUE(std::regex_match(out.date, std::regex(R"(^\d{4}-\d{2}-\d{2}$)")));
    EXPECT_EQ(out.category_id, 5);
}

TEST(Integration_Group2_Fallbacks, EmptyNoteFallsBackToOther) {
    auto out = ProcessTransaction("", "2026-01-02", DefaultCats());
    EXPECT_EQ(out.date, "2026-01-02");
    EXPECT_EQ(out.category_id, 5);
}

TEST(Integration_Group2_Fallbacks, NoOtherCategoryFallsBackToFirst) {
    std::vector<Category> cats = {
        {10, "餐饮", ""},
        {11, "娱乐", ""},
    };
    auto out = ProcessTransaction("完全不匹配", "", cats);
    EXPECT_EQ(out.category_id, 10);
}

TEST(Integration_Group2_Fallbacks, EmptyCategoryListReturns0) {
    std::vector<Category> cats;
    auto out = ProcessTransaction("任意", "", cats);
    EXPECT_TRUE(std::regex_match(out.date, std::regex(R"(^\d{4}-\d{2}-\d{2}$)")));
    EXPECT_EQ(out.category_id, 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
