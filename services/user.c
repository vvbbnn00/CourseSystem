//
// Created by admin on 2022/6/23.
//
// �ر�˵������Ϊ��ҳ���漰���Ĳ��ֲ�����Ҫʵ�ֹ��������ַ��Ȳ�������ʹ��GBK����

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include "socket.h"
#include "global.h"
#include <time.h>
#include "string_ext.h"
#include "utf8_words.h"
#include "ui_utf8.h"
#include "AES.h"
#include "hmacsha256.h"
#include "Windows.h"
#include "link_list_object.h"
#include "ui_gbk.h"
#include "user.h"

const char *USER_ROLE_MAP[] = {"ѧ��", "��ʦ", "����Ա"};
const char *USER_STATUS_MAP[] = {"����", "ͣ��"};
const char *SORT_METHOD_USER[] = {"UID����", "UID����", "��ɫ����", "��ɫ����"};

/**
 * �жϸ��ַ��Ƿ�Ϊ�����ַ�
 * @param c
 * @return �����򷵻�1���������򷵻�0
 */
char inAvailableCharset(char c) {
    const char *available_char = "`~!@#$%^&*()_+-= []{}\\|;:'\",.<>?/";
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return 1;
    for (int i = 0; i < 33; i++) if (c == available_char[i]) return 1;
    return 0;
}


/**
 * ��ȡ���룬����������ʾ
 * @param do_exit ��escȡ�������˳�
 * @return
 */
char *getPassword(char do_exit) {
    char *password = malloc(33); //�洢����
    memset(password, 0, 33);
    int i = 0, t; //��¼���볤��
    char c; //����ʵ��������ʽ����
    while (1) {
        t = _getch();
        if (t == 27) {
            if (do_exit) {
                exit(0);
            } else {
                return "";
            }
        }
        if (t > 127) continue; // ��ASCII�ַ���������
        c = (char) t; //�� _getch() �������룬�ַ�������ʾ����Ļ��
//        printf("%c, %d\n", c);
        if (c == '\r' || c == '\n') { //�����س������������������
            printf("\n");
            break; //while ѭ���ĳ���
        } else if (c == '\b') { //�����˸���Ҫɾ��ǰһ���Ǻ�
            if (i <= 0) continue; // ������Ȳ��㣬��ִ���˸����
            printf("\b \b");  //�˸񣬴�һ���ո����˸�ʵ�������ÿո񸲸ǵ��Ǻ�
            --i;
        } else {
            if (inAvailableCharset(c)) {
                if (i >= 30) continue; // �������볤��
                password[i++] = c;//���ַ���������
                printf("*");
            }
        }
    }
    return password;
}


/**
 * ��ȡ�û��������س�����
 * @return
 */
char *getUsername() {
    char *username = malloc(21); // �û����20λ
    memset(username, 0, 21);
    int i = 0, t;
    char c;
    while (1) {
        t = _getch();
        if (t == 27) exit(0);
        if (t > 127) continue; // ��ASCII�ַ���������
        c = (char) t; //�� _getch() �������룬�ַ�������ʾ����Ļ��
//        printf("%c, %d\n", c);
        if (c == '\r' || c == '\n') { //�����س������������������
            printf("\n");
            break; //while ѭ���ĳ���
        } else if (c == '\b') { //�����˸���Ҫɾ��ǰһ���Ǻ�
            if (i <= 0) continue; // ������Ȳ��㣬��ִ���˸����
            printf("\b \b");  //�˸񣬴�һ���ո����˸�ʵ�������ÿո񸲸ǵ��Ǻ�
            --i;
        } else {
            if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) { // �û���ֻ������ĸ���������ʽ���������ݲ�����
                if (i >= 20) continue; // �����û�������
                username[i++] = c;//���ַ���������
                printf("%c", c);
            }
        }
    }
    return username;
}


/**
 * ��ȡ�û���ɫ
 * @return �û���ɫ
 */
char *getUserRole(int role) {
    switch (role) {
        case 0:
            return WORDS_Login_role_student;
        case 1 :
            return WORDS_Login_role_teacher;
        case 2:
            return WORDS_Login_role_admin;
        default:
            return WORDS_Login_role_unknown;
    }
}


/**
 * ���浱ǰ��¼session
 * @return �ɹ�����1��ʧ�ܷ���0
 */
int Serv_saveLoginSession() {
    FILE *fp = fopen(USER_SESSION, "wb+");
    if (!fp) {
        return 0;
    }
    char key[65];
    strcpy(key, GLOBAL_device_uuid);
    unsigned long long size = sizeof(GLOBAL_user_info);
    unsigned char *source = malloc(size + 16); // Ԥ��16λ���ڲ�λ
    memcpy(source, &GLOBAL_user_info, size);
    unsigned long long padding_length; // ����󳤶�
    AES_Init(key);
    padding_length = AES_add_pkcs7Padding(source, size);
    unsigned char cipherText[padding_length];
    AES_Encrypt(source, cipherText, padding_length, NULL); // AES���ܣ���ԿΪUUID
    fwrite(cipherText, sizeof(unsigned char), padding_length, fp);
    fclose(fp);
    free(source);
    return 1;
}


/**
 * ���ļ���ȡ��¼Session
 * @return ��ȡsession�ɹ�����0����ȡʧ�ܷ���1 - �ļ������� 2 - ��¼ʧЧ
 */
int Serv_getLoginSession() {
    FILE *fp = fopen(USER_SESSION, "rb+");
    if (!fp) {
        return 1; // �ļ�������
    }
    printf(WORDS_Login_trying_autologin);
    // ��ȡSession�ļ�

    char key[65];
    strcpy(key, GLOBAL_device_uuid);
    AES_Init(key);
    char *raw_data = NULL, char_tmp;
    int raw_data_length = 0;
    while (fread(&char_tmp, sizeof(char), 1, fp) != 0) {
        raw_data_length++;
        raw_data = realloc(raw_data, raw_data_length * sizeof(char));
        raw_data[raw_data_length - 1] = char_tmp;
    }
    fclose(fp);
    unsigned char *plainData = calloc(raw_data_length, sizeof(char));
    unsigned char *raw = calloc(raw_data_length, sizeof(unsigned char));
    memcpy(raw, raw_data, raw_data_length);
    AES_Decrypt(plainData, raw, raw_data_length, NULL); // AES����
    AES_delete_pkcs7Padding(plainData, raw_data_length);
    memcpy(&GLOBAL_user_info, plainData, sizeof(GLOBAL_user_info));
    free(plainData);
    free(raw);

    if (time(NULL) > GLOBAL_user_info.expired) {
        // ��¼����
        memset(&GLOBAL_user_info, 0, sizeof(GLOBAL_user_info));
        return 2;
    }

    // ��������
    cJSON *req_json = cJSON_CreateObject();
    ResponseData ret_data = callBackend("user.getUserInfo", req_json);
    cJSON_Delete(req_json);
    if (ret_data.status_code != 0) {  // ��¼ʧ��
        memset(&GLOBAL_user_info, 0, sizeof(GLOBAL_user_info));
        GLOBAL_user_info.role = 3;
        return 2;
    }

    ui_printHeader();
    printf(WORDS_Login_success, GLOBAL_user_info.userid, GLOBAL_user_info.name,
           getUserRole(GLOBAL_user_info.role), getFormatTimeString(GLOBAL_user_info.expired), GLOBAL_user_info.name);
    _getch();
    return 0;
}


/**
 * ִ�е�¼����
 * @param status ��ʾ���ε�¼�Ƿ����Я��״̬
 */
int Serv_login(char status) {
    char *username, *password;
    Login: // ���ñ�ǩ������������򷵻����µ�¼
    system("cls");
    system("@echo off");
    system("chcp 936 > nul & MODE CON COLS=55 LINES=30"); // _getch��bug����Ҫת��ΪGBK����

    printf("\n ----------------------------\n");
    printf("|        ѧ��ѡ��ϵͳ        |\n");
    printf("|   ");
    printf(VERSION);
    printf("   | \n");
    printf(" ----------------------------\n");

    switch (status) {
        case 1:
            printf("[ϵͳ��ʾ] ���ѳɹ��˳���¼��\n");
            break;
        case 2:
            printf("[ϵͳ��ʾ] ���ĵ�¼״̬�ѹ��ڣ������µ�¼��\n");
            break;
        default:
            break;
    }
    status = 0; // ״̬��һ���Ե�

    printf("\n---- �û���¼ - ����\"esc\"��ȡ���� ----\n\n");
    printf("�������û��������س���ȷ�ϣ���\n");
    username = getUsername();
    if (strlen(username) == 0) {
        goto Login;
    }
    printf("���������루���س���ȷ�ϣ���\n");
    password = getPassword(1);
    if (strlen(password) == 0) {
        goto Login;
    }
    if (!regexMatch(PASSWD_PATTERN, password)) {
        printf("��¼ʧ�ܣ����벻��ȷ�����������루����������µ�¼����\n");
        _getch();
        goto Login;
    }
    cJSON *req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "username", cJSON_CreateString(username));
    // ����ͨ��HMAC_SHA256��ʽ���ڿͻ��˼���
    cJSON_AddItemToObject(req_json, "password", cJSON_CreateString(calcHexHMACSHA256(password, "course_system")));
    cJSON_AddItemToObject(req_json, "device_uuid", cJSON_CreateString(GLOBAL_device_uuid));
    ResponseData ret_data = callBackend("user.login", req_json);
    cJSON_Delete(req_json);
    if (ret_data.status_code != 0) {
        printf("��¼ʧ�ܣ������룺%d��������ԭ����:%s ������������µ�¼��\n",
               ret_data.status_code,
               UTF8ToGBK(findExceptionReason(&ret_data)));
        _getch();
        cJSON_Delete(ret_data.json);
        free(ret_data.raw_string);
        goto Login;
    }

    free(username);
    free(password);
    system("chcp 65001  & MODE CON COLS=55 LINES=30 & cls"); // �л���UTF-8����

    // ��������ʱ��
    int ret = jsonParseLong(ret_data.json, "expire", &GLOBAL_user_info.expired);
    if (ret != 0) { // ����ʱ�����ʧ�ܣ���Ĭ��1Сʱ���¼����
        GLOBAL_user_info.expired = time(NULL) + 3600;
    }
    // �����û���
    jsonParseString(ret_data.json, "username", GLOBAL_user_info.userid);
    // ����jwt
    jsonParseString(ret_data.json, "jwt", GLOBAL_user_info.jwt);
    // ��������
    jsonParseString(ret_data.json, "name", GLOBAL_user_info.name);
    // ������ɫ
    ret = jsonParseInteger(ret_data.json, "role", &GLOBAL_user_info.role);
    if (ret != 0) { // ��ɫ����ʧ�ܣ����¼��ֹ
        printf(WORDS_Login_parse_role_error);
        return -1;
    }

    ui_printHeader();
    printf(WORDS_Login_success, GLOBAL_user_info.userid, GLOBAL_user_info.name,
           getUserRole(GLOBAL_user_info.role), getFormatTimeString(GLOBAL_user_info.expired), GLOBAL_user_info.name);

    // �����¼״̬
    if (!Serv_saveLoginSession()) {
        printf(WORDS_Login_save_session_error);
    } else {
        printf(WORDS_Login_status_saved);
    }

    cJSON_Delete(ret_data.json);
    free(ret_data.raw_string);

    _getch();
    return 0;
}

/**
 * �˳���¼����ɾ��Session�ļ����ò��������Ͼ���ɾ��Session�ļ�������ȫ�ֱ������ã����ᷢ�����������
 */
void Serv_logout() {
    memset(&GLOBAL_user_info, 0, sizeof(GLOBAL_user_info));
    GLOBAL_user_info.role = 3;
    remove(USER_SESSION);
}


/**
 * ִ���޸��������
 */
void Serv_changePassword() {
    char *ori_password, *new_password, *new_password_repeat;

    Init:

    system("@echo off");
    system("chcp 936 > nul & MODE CON COLS=55 LINES=30 & cls"); // _getch��bug����Ҫת��ΪGBK����

    printf("\n ----------------------------\n");
    printf("|        ѧ��ѡ��ϵͳ        |\n");
    printf("|   ");
    printf(VERSION);
    printf("   | \n");
    printf(" ----------------------------\n\n");

    printf("\n---- �޸����� - ����\"esc\"�������������Է��أ� ----\n\n");
    printf("�û�����%s\n", GLOBAL_user_info.userid);
    printf("������ԭ���루��\"esc\"�������������Է��أ���\n");
    ori_password = getPassword(0);
    if (strlen(ori_password) == 0) {
        return;
    }
    // ���ñ�ǵ㣬�����벻���Ϲ��������ش˴�
    EnterNewPassword:
    printf("�����������룺\n");
    printf("[����ǿ����ʾ] ����������ͬʱ������ĸ�����֡��ַ��е����ַ��ţ�������8-20λ��\n");
    new_password = getPassword(0);
    if (strlen(new_password) == 0) {
        return;
    }
    if (!regexMatch(PASSWD_PATTERN, new_password)) {
        printf("����ǿ�Ȳ����Ϲ������������롣\n\n");
        goto EnterNewPassword;
    }
    printf("���ٴ����������룺\n");
    new_password_repeat = getPassword(0);
    if (strcmp(new_password_repeat, new_password) != 0) {
        printf("�����������벻��ͬ�����������롣\n\n");
        goto EnterNewPassword;
    }

    if (!regexMatch(PASSWD_PATTERN, ori_password)) {
        printf("[ϵͳ��ʾ] �޸�����ʧ�ܣ�ԭ���벻��ȷ����������������룩\n");
        _getch();
        goto Init;
    }

    cJSON *req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "ori_password",
                          cJSON_CreateString(calcHexHMACSHA256(ori_password, "course_system")));
    cJSON_AddItemToObject(req_json, "new_password",
                          cJSON_CreateString(calcHexHMACSHA256(new_password, "course_system")));
    ResponseData ret_data = callBackend("user.changePassword", req_json);
    cJSON_Delete(req_json);

    if (ret_data.status_code != 0) {
        printf("[ϵͳ��ʾ] �޸�����ʧ�ܣ������룺%d��������ԭ����:%s ����������������룩\n",
               ret_data.status_code,
               UTF8ToGBK(findExceptionReason(&ret_data)));
        _getch();
        cJSON_Delete(ret_data.json);
        free(ret_data.raw_string);
        goto Init;
    }

    printf("[ϵͳ��ʾ] �����޸ĳɹ�����������������˵�����\n");
    _getch();
}


/**
 * �����û����ݣ�����ָ���Ӧ�ṹ��
 *
 * @param json
 * @param _dest
 * @return 0 �ɹ� ���� ʧ��
 */
int parseUserData(cJSON *json, User *_dest) {
    if (jsonParseInteger(json, "role", &_dest->role) ||
        jsonParseLong(json, "last_login_time", &_dest->last_login_time) ||
        jsonParseInteger(json, "status", &_dest->status)) {
        printErrorData("�û���Ϣ���ݽ���ʧ�ܣ��뷵�����ԡ�");
        return -1;
    }
    jsonParseString(json, "uid", _dest->uid);
    jsonParseString(json, "name", _dest->name);
    jsonParseString(json, "last_login_ip", _dest->last_login_ip);
    return 0;
}


/**
 * �����û�
 * @return ȡ���򷵻�NULL�����򷵻��û�ָ��
 */
User *addUser() {
    system("chcp 936>nul & cls & MODE CON COLS=75 LINES=55");
    printf("\n");
    ui_printHeader_GBK(69);
    printf("\n");
    printInMiddle_GBK("======= �û��������û� =======\n", 71);

    User *user = calloc(1, sizeof(User));
    if (inputStringWithRegexCheck("[�����û�] �������û�UID����3-15λ����ĸ��������ɣ�\n",
                                  USER_PATTERN,
                                  user->uid,
                                  15) == -1)
        goto CancelAdd;
    if (inputStringWithRegexCheck("[�����û�] �������û���ʼ���루��8-20Ϊ��ĸ�����֡��ַ���ɣ�������������ͬʱ�������֣�\n",
                                  PASSWD_PATTERN,
                                  user->passwd,
                                  20) == -1)
        goto CancelAdd;
    if (inputStringWithRegexCheck("[�����û�] �������û���������2-20λ�����ġ�Ӣ�ĵ���ɣ�\n",
                                  USER_NAME_PATTERN,
                                  user->name,
                                  20) == -1)
        goto CancelAdd;
    strcpy(user->passwd, calcHexHMACSHA256(GBKToUTF8(user->passwd), "course_system"));
    if (inputIntWithRegexCheck("[�����û�] �������û���ɫ��0-ѧ����1-��ʦ��2-����Ա\n",
                               USER_ROLE_PATTERN,
                               &user->role) == -1)
        goto CancelAdd;
    goto Return;

    CancelAdd:  // ȡ�����
    free(user);
    user = NULL;

    Return:  // ��������
    return user;
}


/**
 * ����/�޸��û�
 *
 * @param _user ��ΪNULLʱ�������û�������Ϊ�޸��û�
 * @return
 */
void editUser(User *_user) {
    // �����û���Ϣ���������û�
    int action = 0;
    User *user = _user;

    if (user == NULL) {
        user = addUser();
        action = 1;
        if (user == NULL) {
            return;
        }
    }

    int counter = 0, selected = 0, key;

    EditUser_Refresh:

    counter = 0;
    system("chcp 936>nul & cls & MODE CON COLS=75 LINES=55");
    printf("\n");
    ui_printHeader_GBK(69);
    printf("\n");
    printInMiddle_GBK("======= �û�������/�޸��û� =======\n\n", 71);

    selfPlusPrint("\t\t�û�UID       %s\n", &counter, selected, UTF8ToGBK(user->uid)); // 0
    selfPlusPrint("\t\t�û�����      %s\n", &counter, selected, UTF8ToGBK(user->name)); // 1
    selfPlusPrint("\t\t�û���ɫ      %s\n", &counter, selected, USER_ROLE_MAP[user->role]); // 2
    selfPlusPrint("\t\t�û�״̬      %s\n", &counter, selected, USER_STATUS_MAP[user->status]); // 3
    selfPlusPrint("\t\t����¼IP    %s\n", &counter, selected, UTF8ToGBK(user->last_login_ip)); // 4
    selfPlusPrint("\t\t����¼ʱ��  %s\n", &counter, selected, getFormatTimeString(user->last_login_time)); // 5
    selfPlusPrint("\t\t�û�����      [������]\n", &counter, selected); // 6

    printf("\n");
    printInMiddle_GBK("<Enter>�޸�ѡ���� <Y>�ύ�޸� <Esc>ȡ���޸�", 71);

    EditCourse_GetKey:

    key = _getch();
    switch (key) {
        case 224:
            key = _getch();
            switch (key) {
                case 80: // ��
                    if (selected < 6) selected++;
                    else selected = 0;
                    goto EditUser_Refresh;
                case 72: // ��
                    if (selected > 0) selected--;
                    else selected = 6;
                    goto EditUser_Refresh;
                default:
                    break;
            }
            break;
        case 13:
            switch (selected) {
                case 1:
                    inputStringWithRegexCheck("[�޸��û�] �������û���������2-20λ�����ġ�Ӣ�ĵ���ɣ�\n",
                                              USER_NAME_PATTERN,
                                              user->name,
                                              20);
                    break;
                case 2:
                    inputIntWithRegexCheck("[�޸��û�] �������û���ɫ��0-ѧ����1-��ʦ��2-����Ա\n",
                                           USER_ROLE_PATTERN,
                                           &user->role);
                    break;
                case 3:
                    user->status = user->status == 0 ? 1 : 0;
                    printf("[�޸��û�] �û����˻�״̬������Ϊ %s\n", USER_ROLE_MAP[user->status]);
                    break;
                case 6:
                    inputStringWithRegexCheck("[�޸��û�] �������û����루��8-20Ϊ��ĸ�����֡��ַ���ɣ�������������ͬʱ�������֣�\n",
                                              PASSWD_PATTERN,
                                              user->passwd,
                                              20);
                    break;
                default:
                    break;
            }
            goto EditUser_Refresh;
        case 'y':
        case 'Y': {
            printf("\n[��ʾ] �������������...\n");
            cJSON *req_json = cJSON_CreateObject();
            cJSON_AddItemToObject(req_json, "action", cJSON_CreateNumber(action));
            cJSON_AddItemToObject(req_json, "uid", cJSON_CreateString(user->uid));
            cJSON_AddItemToObject(req_json, "passwd", cJSON_CreateString(user->passwd));
            cJSON_AddItemToObject(req_json, "name", cJSON_CreateString(user->name));
            cJSON_AddItemToObject(req_json, "role", cJSON_CreateNumber(user->role));
            cJSON_AddItemToObject(req_json, "status", cJSON_CreateNumber(user->status));
            ResponseData resp = callBackend("user.submit", req_json);
            free(req_json);
            free(resp.raw_string);
            if (resp.status_code != 0) {
                printf("[�ύʧ��] �����룺%d, %s(�����������)\n", resp.status_code, UTF8ToGBK(findExceptionReason(&resp)));
                getch();
                cJSON_Delete(resp.json);
                goto EditUser_Refresh;
            }
            cJSON_Delete(resp.json);
            printf("[�ύ�ɹ�] �û��޸�/�����ɹ���������������û��б�\n");
            getch();
            goto GC_COLLECT;
        }
            break;
        case 27:
            goto GC_COLLECT;
        default:
            break;
    }
    goto EditCourse_GetKey;

    GC_COLLECT:
    if (_user == NULL) free(user);
}


/**
 * ���ȫ���û���Ϣ
 */
void printAllUsers() {
    HANDLE windowHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    char search_kw[36] = "";
    int sort_method = 0; // ���򷽷� 0 - UID���� 1 - UID���� 2 - ��ɫ���� 3 - ��ɫ����
    int total, page = 1, max_page, page_size = 15, current_total;

    LinkList_Node *selectedRow = NULL; // ��ѡ�е���
    cJSON *req_json = NULL;
    LinkList_Object *user_data_list = NULL;

    User_GetAndDisplay:

    system("chcp 936>nul & cls & MODE CON COLS=130 LINES=50");
    // ... �������
    printf("[��ʾ] �������������...\n");
    req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "page", cJSON_CreateNumber(page));
    cJSON_AddItemToObject(req_json, "size", cJSON_CreateNumber(page_size));
    cJSON_AddItemToObject(req_json, "kw", cJSON_CreateString(GBKToUTF8(search_kw)));
    cJSON_AddItemToObject(req_json, "sort_method", cJSON_CreateNumber(sort_method));
    ResponseData resp = callBackend("user.getUserList", req_json);
    cJSON_Delete(req_json);
    cJSON *ret_json = resp.json;
    if (resp.status_code != 0) {
        printErrorData(UTF8ToGBK(findExceptionReason(&resp)));
        if (ret_json != NULL) cJSON_Delete(ret_json);
        if (resp.raw_string != NULL) free(resp.raw_string);
        return;
    }
    total = page = current_total = max_page = page_size = -1;

    // ����ҳ��������ݣ�ҳ���
    if (jsonParseInteger(ret_json, "total", &total) ||
        jsonParseInteger(ret_json, "current_total", &current_total) ||
        jsonParseInteger(ret_json, "page", &page) ||
        jsonParseInteger(ret_json, "max_page", &max_page) ||
        jsonParseInteger(ret_json, "page_size", &page_size)) {
        printErrorData("ҳ��������ݽ���ʧ�ܣ��뷵�����ԡ�");
        if (resp.json != NULL) cJSON_Delete(resp.json);
        if (resp.raw_string != NULL) free(resp.raw_string);
        return;
    }

    // �����û�����
    user_data_list = linkListObject_Init(); // �����ʼ��
    cJSON *data = cJSON_GetObjectItem(ret_json, "data");
    for (int i = 0; i < current_total; i++) {
        cJSON *row_data = cJSON_GetArrayItem(data, i);
        User *user_data_pt = calloc(1, sizeof(User));
        if (parseUserData(row_data, user_data_pt)) return;
        LinkList_Node *node = linkListObject_Append(user_data_list, user_data_pt); // ����β��׷��Ԫ��
        if (node == NULL) {
            printErrorData("�ڴ����");
        }
        if (i == 0) selectedRow = node;
    }
    cJSON_Delete(ret_json);
    free(resp.raw_string);

    User_Refresh:

    system("cls");
    printf("\n");
    ui_printHeader_GBK(87);
    printf("\n");
    printInMiddle_GBK("======= �û����û����� =======\n", 89);
    printf("%-17s%-20s%-8s%-6s%-18s%-20s\n", "�û�UID", "����", "��ɫ", "״̬", "����¼IP", "����¼ʱ��");
    printf("-----------------------------------------------------------------------------------------\n");
    for (LinkList_Node *pt = user_data_list->head; pt != NULL; pt = pt->next) {
        User *tmp = pt->data;
        if (pt == selectedRow) SetConsoleTextAttribute(windowHandle, 0x70);
        printf("%-17s%-20s%-8s%-6s%-18s%-20s\n",
               UTF8ToGBK(tmp->uid),
               UTF8ToGBK(tmp->name),
               USER_ROLE_MAP[tmp->role],
               USER_STATUS_MAP[tmp->status],
               UTF8ToGBK(tmp->last_login_ip),
               getFormatTimeString(tmp->last_login_time));
        SetConsoleTextAttribute(windowHandle, 0x07);
        if (pt->next == NULL) { // ��������һ���ڵ�
            printf("\n");
        }
    }
    for (int i = 0; i < page_size - current_total; i++) printf("\n"); // ����ҳ��
    printf("\n");
    printInMiddle_GBK("=============================\n", 89);
    printf("[��ǰ��������] ");
    if (strlen(search_kw) > 0) printf("ģ������=%s AND ", search_kw);
    printf("����ʽ=%s\n", SORT_METHOD_USER[sort_method]);
    printf("[��ʾ] ��%4d�����ݣ���ǰ��%3dҳ����%3dҳ���������ǰһҳ���ҷ��������һҳ����/�·�������л�ѡ�����ݣ�\n",
           total, page, max_page);
    printf("\n  ");
    printf(" <A>�½��û� <Enter>�༭�û� <D>ɾ���û� <K>�û�ģ����ѯ <S>�����л���%s",
           SORT_METHOD_USER[sort_method + 1 > 3 ? 0 : sort_method + 1]);
    printf(" <Esc>�������˵�\n");

    if (selectedRow == NULL) {
        if (strlen(search_kw)) {
            printErrorData("û�в�ѯ�������������û�");
            strcpy(search_kw, "");
            goto User_GetAndDisplay;
        } else {
            printErrorData("�����û������ǲ������ģ�������ε�¼�����ģ���");
            goto GC_Collect;
        }
    }

    int keyboard_press;

    GetKey:
    keyboard_press = _getch();
    switch (keyboard_press) {
        case 224:
            keyboard_press = _getch();
            switch (keyboard_press) {
                case 80: // ��
                    if (selectedRow->next == NULL) selectedRow = user_data_list->head;
                    else selectedRow = selectedRow->next;
                    goto User_Refresh;
                case 72: // ��
                    if (selectedRow->prev == NULL) selectedRow = user_data_list->foot;
                    else selectedRow = selectedRow->prev;
                    goto User_Refresh;
                case 75: // ��
                    page = (page > 1) ? (page - 1) : 1;
                    selectedRow = 0;
                    goto User_GetAndDisplay;
                case 77: // ��
                    page = (page < max_page) ? (page + 1) : (max_page);
                    selectedRow = 0;
                    goto User_GetAndDisplay;
                default:
                    break;
            }
            break;
        case 's': // �޸�����˳��
        case 'S':
            selectedRow = 0;
            sort_method++;
            if (sort_method > 3) sort_method = 0;
            goto User_GetAndDisplay;
        case 'k': // �����ؼ���
        case 'K':
            printf("\n[ģ������] ������ؼ��ʣ���\"Enter\"����������");
            gets_safe(search_kw, 35);
            fflush(stdin);
            selectedRow = 0;
            goto User_GetAndDisplay;
        case 'A':
        case 'a': // �½��û�
            editUser(NULL);
            goto User_GetAndDisplay;
        case 'D':
        case 'd': // ɾ���û�
        {
            User *pt = selectedRow->data;
            printf("\n[ȷ��ɾ��] ��ȷ��Ҫɾ���û� %s(%s) �𣿣�������ɾ���û�������ʹ��ͣ���˻����ܡ���Ҫ��������������û���UID:%sȷ�ϣ�\n",
                   UTF8ToGBK(pt->name),
                   UTF8ToGBK(pt->uid), UTF8ToGBK(pt->uid));
            char input_char[31];
            gets_safe(input_char, 30);
            if (strcmp(input_char, pt->uid) != 0) {
                printf("UID��һ�£���ȡ�����������������������\n");
                getch();
                goto User_GetAndDisplay;
            }
            cJSON *cancel_req_json = cJSON_CreateObject();
            cJSON_AddItemToObject(cancel_req_json, "uid", cJSON_CreateString(pt->uid));
            ResponseData cancel_resp = callBackend("user.deleteUser", cancel_req_json);
            cJSON_Delete(cancel_req_json);
            free(cancel_resp.raw_string);
            if (cancel_resp.status_code != 0) {
                printf("[ɾ��ʧ��] �����룺%d, %s���������������\n", cancel_resp.status_code,
                       UTF8ToGBK(findExceptionReason(&cancel_resp)));
                cJSON_Delete(cancel_resp.json);
                getch();
                goto User_GetAndDisplay;
            }
            printf("[ɾ���ɹ�] �û� %s(%s) �ѱ�ɾ�������������������\n", UTF8ToGBK(pt->name), UTF8ToGBK(pt->uid));
            getch();
            goto User_GetAndDisplay;
        }
        case 13: // �༭�û�
            editUser((User *) selectedRow->data);
            goto User_GetAndDisplay;
        case 27:
            goto GC_Collect;
        default:
            break;
    }
    goto GetKey;

    GC_Collect:
    linkListObject_Delete(user_data_list, 1);
    free(user_data_list);
    user_data_list = NULL;
}
