#include "ImageEditor.hpp"
#include "PhotoMetadataDb.hpp"
#include <chrono>
#include <cstring>
#include <fstream>
#include <gtest/gtest.h>
#include <iomanip>
#include <sstream>

namespace e2e
{

static std::string generate_id()
{
    static int counter = 0;
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::stringstream ss;
    ss << "test_" << ms << "_" << (counter++);
    return ss.str();
}

static std::string get_current_timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

class WorkflowTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite() { db::PhotoMetadataDb::get_instance().initialize(":memory:"); }

    static void TearDownTestSuite() { db::PhotoMetadataDb::get_instance().close(); }

    void SetUp() override {}
    void TearDown() override
    {
        auto photos = db::PhotoMetadataDb::get_instance().get_all_photos();
        for (const auto& photo : photos)
        {
            db::PhotoMetadataDb::get_instance().delete_edit(photo.id);
            db::PhotoMetadataDb::get_instance().delete_photo(photo.id);
        }
    }
};

TEST_F(WorkflowTest, PhotoUpload_InsertPhoto)
{
    db::PhotoMetadata photo;
    photo.id = generate_id();
    photo.filename = "test_upload.jpg";
    photo.filepath = "/uploads/" + photo.filename;
    photo.size = 1024000;
    photo.owner = "test_user";
    photo.created_at = get_current_timestamp();
    photo.updated_at = photo.created_at;
    photo.scene_category = "";
    photo.content_category = "";
    photo.classification_status = "pending";
    photo.classification_dirty = 1;
    photo.classified_at = "";
    photo.schema_version = "1.0";

    bool result = db::PhotoMetadataDb::get_instance().insert_photo(photo);
    EXPECT_TRUE(result);

    auto retrieved = db::PhotoMetadataDb::get_instance().get_photo_by_id(photo.id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->id, photo.id);
    EXPECT_EQ(retrieved->filename, photo.filename);
    EXPECT_EQ(retrieved->classification_status, "pending");
}

TEST_F(WorkflowTest, PhotoUpload_PhotoExists)
{
    db::PhotoMetadata photo;
    photo.id = generate_id();
    photo.filename = "exists_test.jpg";
    photo.filepath = "/uploads/exists_test.jpg";
    photo.size = 500000;
    photo.owner = "test_user";
    photo.created_at = get_current_timestamp();
    photo.updated_at = photo.created_at;
    photo.scene_category = "";
    photo.content_category = "";
    photo.classification_status = "pending";
    photo.classification_dirty = 1;
    photo.classified_at = "";
    photo.schema_version = "1.0";

    EXPECT_FALSE(db::PhotoMetadataDb::get_instance().photo_exists(photo.id));
    db::PhotoMetadataDb::get_instance().insert_photo(photo);
    EXPECT_TRUE(db::PhotoMetadataDb::get_instance().photo_exists(photo.id));
}

TEST_F(WorkflowTest, PhotoUpload_GetPhotosByOwner)
{
    std::string owner = "owner_" + generate_id();

    for (int i = 0; i < 3; ++i)
    {
        db::PhotoMetadata photo;
        photo.id = generate_id();
        photo.filename = "photo_" + std::to_string(i) + ".jpg";
        photo.filepath = "/uploads/photo_" + std::to_string(i) + ".jpg";
        photo.size = 1000000 + i * 100000;
        photo.owner = owner;
        photo.created_at = get_current_timestamp();
        photo.updated_at = photo.created_at;
        photo.scene_category = "";
        photo.content_category = "";
        photo.classification_status = "pending";
        photo.classification_dirty = 1;
        photo.classified_at = "";
        photo.schema_version = "1.0";

        db::PhotoMetadataDb::get_instance().insert_photo(photo);
    }

    auto photos = db::PhotoMetadataDb::get_instance().get_photos_by_owner(owner);
    EXPECT_EQ(photos.size(), 3);
}

TEST_F(WorkflowTest, AutoClassification_UpdatePhotoStatus)
{
    db::PhotoMetadata photo;
    photo.id = generate_id();
    photo.filename = "classify_me.jpg";
    photo.filepath = "/uploads/classify_me.jpg";
    photo.size = 2048000;
    photo.owner = "test_user";
    photo.created_at = get_current_timestamp();
    photo.updated_at = photo.created_at;
    photo.scene_category = "";
    photo.content_category = "";
    photo.classification_status = "pending";
    photo.classification_dirty = 1;
    photo.classified_at = "";
    photo.schema_version = "1.0";

    db::PhotoMetadataDb::get_instance().insert_photo(photo);

    photo.classification_status = "completed";
    photo.scene_category = "landscape";
    photo.content_category = "nature";
    photo.classified_at = get_current_timestamp();
    photo.classification_dirty = 0;

    bool update_result = db::PhotoMetadataDb::get_instance().update_photo(photo);
    EXPECT_TRUE(update_result);

    auto classified = db::PhotoMetadataDb::get_instance().get_photo_by_id(photo.id);
    ASSERT_TRUE(classified.has_value());
    EXPECT_EQ(classified->scene_category, "landscape");
    EXPECT_EQ(classified->content_category, "nature");
    EXPECT_EQ(classified->classification_status, "completed");
}

TEST_F(WorkflowTest, AutoClassification_FailedStatus)
{
    db::PhotoMetadata photo;
    photo.id = generate_id();
    photo.filename = "classify_fail.jpg";
    photo.filepath = "/uploads/classify_fail.jpg";
    photo.size = 100;
    photo.owner = "test_user";
    photo.created_at = get_current_timestamp();
    photo.updated_at = photo.created_at;
    photo.scene_category = "";
    photo.content_category = "";
    photo.classification_status = "processing";
    photo.classification_dirty = 1;
    photo.classified_at = "";
    photo.schema_version = "1.0";

    db::PhotoMetadataDb::get_instance().insert_photo(photo);

    photo.classification_status = "failed";
    photo.updated_at = get_current_timestamp();
    db::PhotoMetadataDb::get_instance().update_photo(photo);

    auto result = db::PhotoMetadataDb::get_instance().get_photo_by_id(photo.id);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->classification_status, "failed");
}

TEST_F(WorkflowTest, EditSave_InsertEdit)
{
    std::string photo_id = generate_id();
    db::PhotoMetadata photo;
    photo.id = photo_id;
    photo.filename = "edit_test.jpg";
    photo.filepath = "/uploads/edit_test.jpg";
    photo.size = 1500000;
    photo.owner = "test_user";
    photo.created_at = get_current_timestamp();
    photo.updated_at = photo.created_at;
    photo.scene_category = "portrait";
    photo.content_category = "people";
    photo.classification_status = "completed";
    photo.classification_dirty = 0;
    photo.classified_at = get_current_timestamp();
    photo.schema_version = "1.0";

    db::PhotoMetadataDb::get_instance().insert_photo(photo);

    EditParams params;
    params.cropX = 10;
    params.cropY = 20;
    params.cropWidth = 800;
    params.cropHeight = 600;
    params.rotation = 90;
    params.brightness = 10;
    params.contrast = 1.5f;
    params.filter = EditParams::Filter::GRAYSCALE;

    std::string edit_json = ImageEditor::toJson(params);

    db::PhotoEdit edit;
    edit.id = generate_id();
    edit.photo_id = photo_id;
    edit.edit_params = edit_json;
    edit.created_at = get_current_timestamp();
    edit.updated_at = edit.created_at;

    bool insert_result = db::PhotoMetadataDb::get_instance().insert_edit(edit);
    EXPECT_TRUE(insert_result);

    auto saved_edit = db::PhotoMetadataDb::get_instance().get_edit_by_photo_id(photo_id);
    ASSERT_TRUE(saved_edit.has_value());
    EXPECT_EQ(saved_edit->photo_id, photo_id);
}

TEST_F(WorkflowTest, EditSave_UpdateEdit)
{
    std::string photo_id = generate_id();
    db::PhotoMetadata photo;
    photo.id = photo_id;
    photo.filename = "edit_update.jpg";
    photo.filepath = "/uploads/edit_update.jpg";
    photo.size = 1500000;
    photo.owner = "test_user";
    photo.created_at = get_current_timestamp();
    photo.updated_at = photo.created_at;
    photo.scene_category = "landscape";
    photo.content_category = "nature";
    photo.classification_status = "completed";
    photo.classification_dirty = 0;
    photo.classified_at = get_current_timestamp();
    photo.schema_version = "1.0";

    db::PhotoMetadataDb::get_instance().insert_photo(photo);

    EditParams initial_params;
    initial_params.rotation = 90;
    std::string initial_json = ImageEditor::toJson(initial_params);

    db::PhotoEdit edit;
    edit.id = generate_id();
    edit.photo_id = photo_id;
    edit.edit_params = initial_json;
    edit.created_at = get_current_timestamp();
    edit.updated_at = edit.created_at;

    db::PhotoMetadataDb::get_instance().insert_edit(edit);

    EditParams new_params;
    new_params.rotation = 180;
    new_params.brightness = 5;
    new_params.filter = EditParams::Filter::SEPIA;
    std::string new_json = ImageEditor::toJson(new_params);

    edit.edit_params = new_json;
    edit.updated_at = get_current_timestamp();

    bool update_result = db::PhotoMetadataDb::get_instance().update_edit(edit);
    EXPECT_TRUE(update_result);

    auto updated_edit = db::PhotoMetadataDb::get_instance().get_edit_by_photo_id(photo_id);
    ASSERT_TRUE(updated_edit.has_value());

    auto parsed = ImageEditor::fromJson(updated_edit->edit_params);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->rotation, 180);
    EXPECT_EQ(parsed->brightness, 5);
    EXPECT_EQ(parsed->filter, EditParams::Filter::SEPIA);
}

TEST_F(WorkflowTest, GetEditParams_RetrieveEdit)
{
    std::string photo_id = generate_id();
    db::PhotoMetadata photo;
    photo.id = photo_id;
    photo.filename = "get_edit.jpg";
    photo.filepath = "/uploads/get_edit.jpg";
    photo.size = 2000000;
    photo.owner = "test_user";
    photo.created_at = get_current_timestamp();
    photo.updated_at = photo.created_at;
    photo.scene_category = "architecture";
    photo.content_category = "building";
    photo.classification_status = "completed";
    photo.classification_dirty = 0;
    photo.classified_at = get_current_timestamp();
    photo.schema_version = "1.0";

    db::PhotoMetadataDb::get_instance().insert_photo(photo);

    EditParams params;
    params.cropX = 0;
    params.cropY = 0;
    params.cropWidth = 1920;
    params.cropHeight = 1080;
    params.targetWidth = 800;
    params.targetHeight = 600;
    params.rotation = 0;
    params.brightness = -5;
    params.contrast = 1.2f;
    params.filter = EditParams::Filter::VINTAGE;

    std::string edit_json = ImageEditor::toJson(params);

    db::PhotoEdit edit;
    edit.id = generate_id();
    edit.photo_id = photo_id;
    edit.edit_params = edit_json;
    edit.created_at = get_current_timestamp();
    edit.updated_at = edit.created_at;

    db::PhotoMetadataDb::get_instance().insert_edit(edit);

    auto retrieved_edit = db::PhotoMetadataDb::get_instance().get_edit_by_photo_id(photo_id);
    ASSERT_TRUE(retrieved_edit.has_value());

    auto parsed_params = ImageEditor::fromJson(retrieved_edit->edit_params);
    ASSERT_TRUE(parsed_params.has_value());
    EXPECT_EQ(parsed_params->cropWidth, 1920);
    EXPECT_EQ(parsed_params->cropHeight, 1080);
    EXPECT_EQ(parsed_params->targetWidth, 800);
    EXPECT_EQ(parsed_params->targetHeight, 600);
    EXPECT_EQ(parsed_params->brightness, -5);
    EXPECT_FLOAT_EQ(parsed_params->contrast, 1.2f);
    EXPECT_EQ(parsed_params->filter, EditParams::Filter::VINTAGE);
}

TEST_F(WorkflowTest, GetEditParams_NoEdit)
{
    std::string photo_id = generate_id();
    db::PhotoMetadata photo;
    photo.id = photo_id;
    photo.filename = "no_edit.jpg";
    photo.filepath = "/uploads/no_edit.jpg";
    photo.size = 1000000;
    photo.owner = "test_user_edit";
    photo.created_at = get_current_timestamp();
    photo.updated_at = photo.created_at;
    photo.scene_category = "nature";
    photo.content_category = "landscape";
    photo.classification_status = "completed";
    photo.classification_dirty = 0;
    photo.classified_at = get_current_timestamp();
    photo.schema_version = "1.0";

    db::PhotoMetadataDb::get_instance().insert_photo(photo);

    auto non_existent = db::PhotoMetadataDb::get_instance().get_edit_by_photo_id(photo_id);
    EXPECT_FALSE(non_existent.has_value());
}

TEST_F(WorkflowTest, RevertEdits_DeleteEdit)
{
    std::string photo_id = generate_id();
    db::PhotoMetadata photo;
    photo.id = photo_id;
    photo.filename = "revert_test.jpg";
    photo.filepath = "/uploads/revert_test.jpg";
    photo.size = 1800000;
    photo.owner = "test_user";
    photo.created_at = get_current_timestamp();
    photo.updated_at = photo.created_at;
    photo.scene_category = "portrait";
    photo.content_category = "people";
    photo.classification_status = "completed";
    photo.classification_dirty = 0;
    photo.classified_at = get_current_timestamp();
    photo.schema_version = "1.0";

    db::PhotoMetadataDb::get_instance().insert_photo(photo);

    EditParams params;
    params.rotation = 270;
    params.brightness = 10;

    db::PhotoEdit edit;
    edit.id = generate_id();
    edit.photo_id = photo_id;
    edit.edit_params = ImageEditor::toJson(params);
    edit.created_at = get_current_timestamp();
    edit.updated_at = edit.created_at;

    db::PhotoMetadataDb::get_instance().insert_edit(edit);

    bool delete_result = db::PhotoMetadataDb::get_instance().delete_edit(photo_id);
    EXPECT_TRUE(delete_result);

    auto deleted = db::PhotoMetadataDb::get_instance().get_edit_by_photo_id(photo_id);
    EXPECT_FALSE(deleted.has_value());
}

TEST_F(WorkflowTest, RevertEdits_DeleteNonExistent)
{
    std::string photo_id = "nonexistent_revert";

    bool result = db::PhotoMetadataDb::get_instance().delete_edit(photo_id);
    EXPECT_TRUE(result);
}

TEST_F(WorkflowTest, CompleteWorkflow_UploadClassifyEditGetRevert)
{
    std::string photo_id = generate_id();
    db::PhotoMetadata photo;
    photo.id = photo_id;
    photo.filename = "workflow_complete.jpg";
    photo.filepath = "/uploads/workflow_complete.jpg";
    photo.size = 2500000;
    photo.owner = "workflow_user";
    photo.created_at = get_current_timestamp();
    photo.updated_at = photo.created_at;
    photo.scene_category = "";
    photo.content_category = "";
    photo.classification_status = "pending";
    photo.classification_dirty = 1;
    photo.classified_at = "";
    photo.schema_version = "1.0";

    bool upload_result = db::PhotoMetadataDb::get_instance().insert_photo(photo);
    ASSERT_TRUE(upload_result);

    auto uploaded = db::PhotoMetadataDb::get_instance().get_photo_by_id(photo_id);
    ASSERT_TRUE(uploaded.has_value());
    EXPECT_EQ(uploaded->classification_status, "pending");

    photo.classification_status = "completed";
    photo.scene_category = "landscape";
    photo.content_category = "nature";
    photo.classified_at = get_current_timestamp();
    photo.classification_dirty = 0;
    photo.updated_at = get_current_timestamp();

    bool classify_result = db::PhotoMetadataDb::get_instance().update_photo(photo);
    ASSERT_TRUE(classify_result);

    auto classified = db::PhotoMetadataDb::get_instance().get_photo_by_id(photo_id);
    ASSERT_TRUE(classified.has_value());
    EXPECT_EQ(classified->scene_category, "landscape");
    EXPECT_EQ(classified->content_category, "nature");

    EditParams edit_params;
    edit_params.cropX = 50;
    edit_params.cropY = 50;
    edit_params.cropWidth = 1600;
    edit_params.cropHeight = 900;
    edit_params.rotation = 90;
    edit_params.brightness = 5;
    edit_params.contrast = 1.3f;
    edit_params.filter = EditParams::Filter::GRAYSCALE;

    db::PhotoEdit edit;
    edit.id = generate_id();
    edit.photo_id = photo_id;
    edit.edit_params = ImageEditor::toJson(edit_params);
    edit.created_at = get_current_timestamp();
    edit.updated_at = edit.created_at;

    bool save_result = db::PhotoMetadataDb::get_instance().insert_edit(edit);
    ASSERT_TRUE(save_result);

    auto retrieved_edit = db::PhotoMetadataDb::get_instance().get_edit_by_photo_id(photo_id);
    ASSERT_TRUE(retrieved_edit.has_value());

    auto parsed = ImageEditor::fromJson(retrieved_edit->edit_params);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->cropX, 50);
    EXPECT_EQ(parsed->rotation, 90);
    EXPECT_EQ(parsed->filter, EditParams::Filter::GRAYSCALE);

    bool revert_result = db::PhotoMetadataDb::get_instance().delete_edit(photo_id);
    ASSERT_TRUE(revert_result);

    auto reverted = db::PhotoMetadataDb::get_instance().get_edit_by_photo_id(photo_id);
    EXPECT_FALSE(reverted.has_value());

    auto photo_still_exists = db::PhotoMetadataDb::get_instance().get_photo_by_id(photo_id);
    ASSERT_TRUE(photo_still_exists.has_value());
}

TEST_F(WorkflowTest, CompleteWorkflow_MultiplePhotos)
{
    std::string owner = "multi_user";
    std::vector<std::string> photo_ids;

    for (int i = 0; i < 5; ++i)
    {
        std::string photo_id = generate_id();
        db::PhotoMetadata photo;
        photo.id = photo_id;
        photo.filename = "multi_" + std::to_string(i) + ".jpg";
        photo.filepath = "/uploads/multi_" + std::to_string(i) + ".jpg";
        photo.size = 1000000 + i * 500000;
        photo.owner = owner;
        photo.created_at = get_current_timestamp();
        photo.updated_at = photo.created_at;
        photo.scene_category = "";
        photo.content_category = "";
        photo.classification_status = "pending";
        photo.classification_dirty = 1;
        photo.classified_at = "";
        photo.schema_version = "1.0";

        db::PhotoMetadataDb::get_instance().insert_photo(photo);
        photo_ids.push_back(photo_id);
    }

    std::vector<std::string> categories = {"landscape", "portrait", "nature", "architecture", "street"};
    for (size_t i = 0; i < photo_ids.size(); ++i)
    {
        auto photo = db::PhotoMetadataDb::get_instance().get_photo_by_id(photo_ids[i]);
        ASSERT_TRUE(photo.has_value());

        photo->classification_status = "completed";
        photo->scene_category = categories[i];
        photo->content_category = "general";
        photo->classified_at = get_current_timestamp();
        photo->classification_dirty = 0;
        photo->updated_at = get_current_timestamp();

        db::PhotoMetadataDb::get_instance().update_photo(*photo);
    }

    for (size_t i = 0; i < photo_ids.size(); ++i)
    {
        EditParams params;
        params.rotation = (i % 4) * 90;
        params.targetWidth = 800;
        params.targetHeight = 600;

        db::PhotoEdit edit;
        edit.id = generate_id();
        edit.photo_id = photo_ids[i];
        edit.edit_params = ImageEditor::toJson(params);
        edit.created_at = get_current_timestamp();
        edit.updated_at = edit.created_at;

        db::PhotoMetadataDb::get_instance().insert_edit(edit);
    }

    auto photos = db::PhotoMetadataDb::get_instance().get_photos_by_owner(owner);
    EXPECT_EQ(photos.size(), 5);

    for (const auto& pid : photo_ids)
    {
        auto edit = db::PhotoMetadataDb::get_instance().get_edit_by_photo_id(pid);
        ASSERT_TRUE(edit.has_value());
    }

    for (const auto& pid : photo_ids)
    {
        db::PhotoMetadataDb::get_instance().delete_edit(pid);
    }

    for (const auto& pid : photo_ids)
    {
        auto edit = db::PhotoMetadataDb::get_instance().get_edit_by_photo_id(pid);
        EXPECT_FALSE(edit.has_value());
    }
}

} // namespace e2e

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}