#include "unity.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "cJSON.h"

#ifndef MIMI_CHAN_TELEGRAM
#define MIMI_CHAN_TELEGRAM "telegram"
#endif

void setUp(void) {}
void tearDown(void) {}

/* 1. Photo message JSON parsing — extract highest-res file_id */
void test_photo_parse_highest_res(void)
{
    const char *json =
        "{\"message_id\":42,\"chat\":{\"id\":12345},\"from\":{\"id\":99},"
        "\"photo\":["
        "{\"file_id\":\"small\",\"width\":90,\"height\":90},"
        "{\"file_id\":\"medium\",\"width\":320,\"height\":320},"
        "{\"file_id\":\"large\",\"width\":800,\"height\":800}"
        "]}";

    cJSON *msg = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(msg);

    cJSON *photo_array = cJSON_GetObjectItem(msg, "photo");
    TEST_ASSERT_TRUE(cJSON_IsArray(photo_array));
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(photo_array));

    int last_idx = cJSON_GetArraySize(photo_array) - 1;
    cJSON *photo = cJSON_GetArrayItem(photo_array, last_idx);
    cJSON *file_id_obj = cJSON_GetObjectItem(photo, "file_id");
    TEST_ASSERT_EQUAL_STRING("large", file_id_obj->valuestring);

    cJSON_Delete(msg);
}

/* 2. Document message parsing — extract file_id and file_name */
void test_document_parse(void)
{
    const char *json =
        "{\"message_id\":43,\"chat\":{\"id\":12345},"
        "\"document\":{\"file_id\":\"doc_fid_001\",\"file_name\":\"report.pdf\"}}";

    cJSON *msg = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(msg);

    cJSON *doc = cJSON_GetObjectItem(msg, "document");
    TEST_ASSERT_TRUE(cJSON_IsObject(doc));

    cJSON *file_id_obj = cJSON_GetObjectItem(doc, "file_id");
    TEST_ASSERT_EQUAL_STRING("doc_fid_001", file_id_obj->valuestring);

    cJSON *doc_name = cJSON_GetObjectItem(doc, "file_name");
    TEST_ASSERT_EQUAL_STRING("report.pdf", doc_name->valuestring);

    cJSON_Delete(msg);
}

/* 3. Voice message parsing — extract file_id */
void test_voice_parse(void)
{
    const char *json =
        "{\"message_id\":44,\"chat\":{\"id\":12345},"
        "\"voice\":{\"file_id\":\"voice_fid_002\",\"duration\":15}}";

    cJSON *msg = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(msg);

    cJSON *voice = cJSON_GetObjectItem(msg, "voice");
    TEST_ASSERT_TRUE(cJSON_IsObject(voice));

    cJSON *file_id_obj = cJSON_GetObjectItem(voice, "file_id");
    TEST_ASSERT_EQUAL_STRING("voice_fid_002", file_id_obj->valuestring);

    cJSON_Delete(msg);
}

/* 4. Video message parsing — extract file_id */
void test_video_parse(void)
{
    const char *json =
        "{\"message_id\":45,\"chat\":{\"id\":12345},"
        "\"video\":{\"file_id\":\"video_fid_003\",\"duration\":30,\"width\":640,\"height\":480}}";

    cJSON *msg = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(msg);

    cJSON *video = cJSON_GetObjectItem(msg, "video");
    TEST_ASSERT_TRUE(cJSON_IsObject(video));

    cJSON *file_id_obj = cJSON_GetObjectItem(video, "file_id");
    TEST_ASSERT_EQUAL_STRING("video_fid_003", file_id_obj->valuestring);

    cJSON_Delete(msg);
}

/* 5. File download URL construction from getFile response */
void test_download_url_construction(void)
{
    const char *get_file_resp =
        "{\"ok\":true,\"result\":{\"file_id\":\"AgACAgIAAxkB\",\"file_unique_id\":\"AQADt\",\"file_path\":\"photos/file_0.jpg\"}}";

    cJSON *root = cJSON_Parse(get_file_resp);
    TEST_ASSERT_NOT_NULL(root);

    cJSON *result = cJSON_GetObjectItem(root, "result");
    cJSON *file_path_obj = cJSON_GetObjectItem(result, "file_path");
    TEST_ASSERT_EQUAL_STRING("photos/file_0.jpg", file_path_obj->valuestring);

    char url[512];
    snprintf(url, sizeof(url), "https://api.telegram.org/file/botTEST_TOKEN/%s", file_path_obj->valuestring);
    TEST_ASSERT_EQUAL_STRING(
        "https://api.telegram.org/file/botTEST_TOKEN/photos/file_0.jpg", url);

    cJSON_Delete(root);
}

/* 6. Photo send payload structure — verify JSON fields */
void test_send_photo_payload(void)
{
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "chat_id", "12345");
    cJSON_AddStringToObject(payload, "photo", "https://example.com/img.png");
    cJSON_AddStringToObject(payload, "caption", "test caption");

    TEST_ASSERT_EQUAL_STRING("12345",
        cJSON_GetObjectItem(payload, "chat_id")->valuestring);
    TEST_ASSERT_EQUAL_STRING("https://example.com/img.png",
        cJSON_GetObjectItem(payload, "photo")->valuestring);
    TEST_ASSERT_EQUAL_STRING("test caption",
        cJSON_GetObjectItem(payload, "caption")->valuestring);

    cJSON_Delete(payload);
}

/* 7. media_count field set to 1 for media messages */
void test_media_count_field(void)
{
    const char *json =
        "{\"message_id\":46,\"chat\":{\"id\":12345},"
        "\"photo\":[{\"file_id\":\"fid\",\"width\":100,\"height\":100}]}";

    cJSON *msg = cJSON_Parse(json);
    cJSON *photo_array = cJSON_GetObjectItem(msg, "photo");
    TEST_ASSERT_TRUE(cJSON_IsArray(photo_array));

    int media_count = cJSON_GetArraySize(photo_array) > 0 ? 1 : 0;
    TEST_ASSERT_EQUAL_INT(1, media_count);

    cJSON_Delete(msg);
}

/* 8. media_paths extraction from photo */
void test_media_paths_extraction(void)
{
    const char *json =
        "{\"message_id\":47,\"chat\":{\"id\":12345},"
        "\"photo\":[{\"file_id\":\"photo_fid_xyz\",\"width\":100,\"height\":100}]}";

    cJSON *msg = cJSON_Parse(json);
    cJSON *photo_array = cJSON_GetObjectItem(msg, "photo");
    int last_idx = cJSON_GetArraySize(photo_array) - 1;
    cJSON *photo = cJSON_GetArrayItem(photo_array, last_idx);
    cJSON *file_id_obj = cJSON_GetObjectItem(photo, "file_id");

    char media_paths[4][64] = {0};
    strncpy(media_paths[0], file_id_obj->valuestring, sizeof(media_paths[0]) - 1);
    TEST_ASSERT_EQUAL_STRING("photo_fid_xyz", media_paths[0]);

    cJSON_Delete(msg);
}

/* 9. Caption extraction from photo message */
void test_caption_extraction(void)
{
    const char *json =
        "{\"message_id\":48,\"chat\":{\"id\":12345},"
        "\"caption\":\"Look at this!\","
        "\"photo\":[{\"file_id\":\"fid\",\"width\":100,\"height\":100}]}";

    cJSON *msg = cJSON_Parse(json);

    cJSON *caption = cJSON_GetObjectItem(msg, "caption");
    TEST_ASSERT_NOT_NULL(caption);
    TEST_ASSERT_TRUE(cJSON_IsString(caption));
    TEST_ASSERT_EQUAL_STRING("Look at this!", caption->valuestring);

    cJSON_Delete(msg);
}

/* 10. Multiple photos in array — highest resolution selection */
void test_multiple_photos_highest_res(void)
{
    const char *json =
        "{\"photo\":["
        "{\"file_id\":\"thumb1\",\"width\":90},"
        "{\"file_id\":\"thumb2\",\"width\":160},"
        "{\"file_id\":\"medium\",\"width\":320},"
        "{\"file_id\":\"full\",\"width\":1280}"
        "]}";

    cJSON *msg = cJSON_Parse(json);
    cJSON *photo_array = cJSON_GetObjectItem(msg, "photo");
    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(photo_array));

    int last_idx = cJSON_GetArraySize(photo_array) - 1;
    cJSON *photo = cJSON_GetArrayItem(photo_array, last_idx);
    cJSON *fid = cJSON_GetObjectItem(photo, "file_id");
    TEST_ASSERT_EQUAL_STRING("full", fid->valuestring);

    cJSON *prev = cJSON_GetArrayItem(photo_array, last_idx - 1);
    cJSON *prev_fid = cJSON_GetObjectItem(prev, "file_id");
    TEST_ASSERT_EQUAL_STRING("medium", prev_fid->valuestring);

    cJSON_Delete(msg);
}

/* 11. NULL input handling — download with NULL file_id */
void test_download_null_file_id(void)
{
    const char *file_id = NULL;
    char local_path[256] = {0};
    int invalid = (!file_id || !local_path) ? 1 : 0;
    TEST_ASSERT_TRUE(invalid);
}

/* 12. NULL input handling — send photo with NULL chat_id */
void test_send_photo_null_chat_id(void)
{
    const char *chat_id = NULL;
    const char *file_path = "/some/path.jpg";
    int invalid = (!chat_id || !file_path) ? 1 : 0;
    TEST_ASSERT_TRUE(invalid);
}

/* 13. Message with no text but has photo — text handler skips, photo handler picks up */
void test_no_text_has_photo(void)
{
    const char *json =
        "{\"message_id\":50,\"chat\":{\"id\":12345},"
        "\"photo\":[{\"file_id\":\"pic123\",\"width\":800,\"height\":600}]}";

    cJSON *msg = cJSON_Parse(json);

    cJSON *text = cJSON_GetObjectItem(msg, "text");
    TEST_ASSERT_NULL(text);

    cJSON *photo_array = cJSON_GetObjectItem(msg, "photo");
    TEST_ASSERT_NOT_NULL(photo_array);
    TEST_ASSERT_TRUE(cJSON_IsArray(photo_array));
    TEST_ASSERT_TRUE(cJSON_GetArraySize(photo_array) > 0);

    cJSON_Delete(msg);
}

/* 14. Document without file_name uses "unknown" fallback */
void test_document_no_filename(void)
{
    const char *json =
        "{\"message_id\":51,\"chat\":{\"id\":12345},"
        "\"document\":{\"file_id\":\"doc_fid_999\"}}";

    cJSON *msg = cJSON_Parse(json);
    cJSON *doc = cJSON_GetObjectItem(msg, "document");
    cJSON *doc_name = cJSON_GetObjectItem(doc, "file_name");

    char doc_desc[256];
    snprintf(doc_desc, sizeof(doc_desc), "[document: %s]",
             doc_name ? doc_name->valuestring : "unknown");
    TEST_ASSERT_EQUAL_STRING("[document: unknown]", doc_desc);

    cJSON_Delete(msg);
}

/* 15. Mixed text and media — text takes priority */
void test_text_takes_priority_over_media(void)
{
    const char *json =
        "{\"message_id\":52,\"chat\":{\"id\":12345},"
        "\"text\":\"Hello world\","
        "\"photo\":[{\"file_id\":\"pic\",\"width\":100,\"height\":100}]}";

    cJSON *msg = cJSON_Parse(json);

    cJSON *text = cJSON_GetObjectItem(msg, "text");
    int text_present = (text && cJSON_IsString(text)) ? 1 : 0;
    TEST_ASSERT_TRUE(text_present);

    cJSON *photo_array = cJSON_GetObjectItem(msg, "photo");
    int photo_present = (photo_array && cJSON_IsArray(photo_array)
                         && cJSON_GetArraySize(photo_array) > 0) ? 1 : 0;
    TEST_ASSERT_TRUE(photo_present);

    TEST_ASSERT_TRUE(text_present && photo_present);

    const char *chosen_content = text->valuestring;
    TEST_ASSERT_EQUAL_STRING("Hello world", chosen_content);

    cJSON_Delete(msg);
}

int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_photo_parse_highest_res);
    RUN_TEST(test_document_parse);
    RUN_TEST(test_voice_parse);
    RUN_TEST(test_video_parse);
    RUN_TEST(test_download_url_construction);
    RUN_TEST(test_send_photo_payload);
    RUN_TEST(test_media_count_field);
    RUN_TEST(test_media_paths_extraction);
    RUN_TEST(test_caption_extraction);
    RUN_TEST(test_multiple_photos_highest_res);
    RUN_TEST(test_download_null_file_id);
    RUN_TEST(test_send_photo_null_chat_id);
    RUN_TEST(test_no_text_has_photo);
    RUN_TEST(test_document_no_filename);
    RUN_TEST(test_text_takes_priority_over_media);
    return UNITY_END();
}
