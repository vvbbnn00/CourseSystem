//
// Created by admin on 2022/7/2.
//

#include "system_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "ui_gbk.h"
#include "global.h"
#include "string_ext.h"
#include "link_list_object.h"
#include <Windows.h>

const char *SORT_METHOD_SEMESTER[] = {"ѧ�ڽ���", "ѧ������"};

/**
 * ����ѧ���������ݣ�����ָ���Ӧ�ṹ��
 *
 * @param json
 * @param _dest
 * @return 0 �ɹ� ���� ʧ��
 */
int parseSemesterData(cJSON *json, SemesterLimit *_dest) {
    if (jsonParseFloat(json, "max_points", &_dest->max_points)) {
        printErrorData("ѧ����Ϣ���ݽ���ʧ�ܣ��뷵�����ԡ�");
        return -1;
    }
    jsonParseString(json, "semester", _dest->semester);
    return 0;
}


/**
 * ����ѧ������
 * @return ȡ���򷵻�NULL�����򷵻�ѧ������ָ��
 */
SemesterLimit *addSemesterLimit() {
    system("chcp 936>nul & cls & MODE CON COLS=75 LINES=55");
    printf("\n");
    ui_printHeader_GBK(69);
    printf("\n");
    printInMiddle_GBK("======= ϵͳ���á�����ѧ������ =======\n", 71);

    SemesterLimit *semester = calloc(1, sizeof(SemesterLimit));
    if (inputStringWithRegexCheck("[����ѧ��] ������ѧ�ڣ�����Ϊ 4-10 �ַ�������ĸ�����ֺ�-��ɵ��ַ����������ʽ<ѧ��>-<ѧ��>��\n",
                                  SEMESTER_PATTERN,
                                  semester->semester,
                                  10) == -1)
        goto CancelAdd;
    if (inputFloatWithRegexCheck("[����ѧ��] ������ѧ�����ѧ�֣���߾�ȷ����λС��\n",
                                 POINTS_PATTERN,
                                 &semester->max_points) == -1)
        goto CancelAdd;
    goto Return;

    CancelAdd:  // ȡ�����
    free(semester);
    semester = NULL;

    Return:  // ��������
    return semester;
}


/**
 * ����/�޸�ѧ������
 *
 * @param _semester ��ΪNULLʱ������ѧ�����ã�����Ϊ�޸�ѧ������
 * @return �γ�ָ��
 */
void editSemester(SemesterLimit *_semester) {
    // ����ѧ����Ϣ��������ѧ��
    int action = 0;
    SemesterLimit *semester = _semester;

    if (semester == NULL) {
        semester = addSemesterLimit();
        action = 1;
        if (semester == NULL) {
            return;
        }
    }

    int counter = 0, selected = 0, key;

    EditSemester_Refresh:

    counter = 0;
    system("chcp 936>nul & cls & MODE CON COLS=75 LINES=55");
    printf("\n");
    ui_printHeader_GBK(69);
    printf("\n");
    printInMiddle_GBK("======= ϵͳ���á�����/�޸�ѧ������ =======\n\n", 71);

    selfPlusPrint("\t\tѧ��           %s\n", &counter, selected, UTF8ToGBK(semester->semester)); // 0
    selfPlusPrint("\t\t���ѧ������   %.2f\n", &counter, selected, semester->max_points); // 1
    printf("\n");
    printInMiddle_GBK("<Enter>�޸�ѡ���� <Y>�ύ�޸� <Esc>ȡ���޸�", 71);

    EditSemester_GetKey:

    key = _getch();
    switch (key) {
        case 224:
            key = _getch();
            switch (key) {
                case 80: // ��
                    if (selected < 1) selected++;
                    else selected = 0;
                    goto EditSemester_Refresh;
                case 72: // ��
                    if (selected > 0) selected--;
                    else selected = 1;
                    goto EditSemester_Refresh;
                default:
                    break;
            }
            break;
        case 13:
            if (selected == 1) {
                inputFloatWithRegexCheck("[�޸�ѧ������] ������ѧ�����ѧ�֣���߾�ȷ����λС��\n",
                                         POINTS_PATTERN,
                                         &semester->max_points);
            }
            goto EditSemester_Refresh;
        case 'y':
        case 'Y': {
            printf("\n[��ʾ] �������������...\n");
            cJSON *req_json = cJSON_CreateObject();
            cJSON_AddItemToObject(req_json, "action", cJSON_CreateNumber(action));
            cJSON_AddItemToObject(req_json, "semester", cJSON_CreateString(semester->semester));
            cJSON_AddItemToObject(req_json, "max_points", cJSON_CreateNumber(semester->max_points));
            ResponseData resp = callBackend("semester.submit", req_json);
            free(req_json);
            free(resp.raw_string);
            if (resp.status_code != 0) {
                printf("[�ύʧ��] �����룺%d, %s(�����������)\n", resp.status_code, UTF8ToGBK(findExceptionReason(&resp)));
                getch();
                cJSON_Delete(resp.json);
                goto EditSemester_Refresh;
            }
            cJSON_Delete(resp.json);
            printf("[�ύ�ɹ�] ѧ����Ϣ�޸�/�����ɹ�������������أ�\n");
            getch();
            goto GC_COLLECT;
        }
        case 27:
            goto GC_COLLECT;
        default:
            break;
    }
    goto EditSemester_GetKey;

    GC_COLLECT:
    if (_semester == NULL) free(semester);
}


/**
 * ���ѧ����Ϣ��
 */
void printSemesterData() {
    HANDLE windowHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    char search_kw[36] = "";
    int sort_method = 0; // ���򷽷� 0 - ѧ������ 1 - ѧ�ڽ���
    int total, page = 1, max_page, page_size = 15, current_total;

    LinkList_Node *selectedRow = NULL; // ��ѡ�е���
    cJSON *req_json = NULL;
    LinkList_Object *semester_data_list = NULL;

    Semester_GetAndDisplay:

    system("chcp 936>nul & cls & MODE CON COLS=60 LINES=50");
    // ... �������
    printf("[��ʾ] �������������...\n");
    req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "page", cJSON_CreateNumber(page));
    cJSON_AddItemToObject(req_json, "size", cJSON_CreateNumber(page_size));
    cJSON_AddItemToObject(req_json, "kw", cJSON_CreateString(GBKToUTF8(search_kw)));
    cJSON_AddItemToObject(req_json, "sort_method", cJSON_CreateNumber(sort_method));
    ResponseData resp = callBackend("semester.getLimitList", req_json);
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
    jsonParseString(ret_json, "school", GLOBAL_school);
    jsonParseString(ret_json, "current_semester", GLOBAL_semester);

    // ����ѧ����Ϣ����
    semester_data_list = linkListObject_Init(); // �����ʼ��
    cJSON *data = cJSON_GetObjectItem(ret_json, "data");
    for (int i = 0; i < current_total; i++) {
        cJSON *row_data = cJSON_GetArrayItem(data, i);
        SemesterLimit *semester_data_pt = calloc(1, sizeof(SemesterLimit));
        if (parseSemesterData(row_data, semester_data_pt)) return;
        LinkList_Node *node = linkListObject_Append(semester_data_list, semester_data_pt); // ����β��׷��Ԫ��
        if (node == NULL) {
            printErrorData("�ڴ����");
        }
        if (i == 0) selectedRow = node;
    }
    cJSON_Delete(ret_json);
    free(resp.raw_string);

    Semester_Refresh:

    system("cls");
    printf("\n");
    ui_printHeader_GBK(38);
    printf("\n");
    printInMiddle_GBK("======= ϵͳ���á�ѧ����Ϣһ�� =======\n", 38);
    printf("       %-14s%-10s       \n", "ѧ��", "����ѡѧ��");
    printf("      --------------------------\n");
    for (LinkList_Node *pt = semester_data_list->head; pt != NULL; pt = pt->next) {
        SemesterLimit *tmp = pt->data;
        printf("      ");
        if (pt == selectedRow) SetConsoleTextAttribute(windowHandle, 0x70);
        printf("%-14s%-10.2f\n",
               UTF8ToGBK(tmp->semester),
               tmp->max_points);
        SetConsoleTextAttribute(windowHandle, 0x07);
        if (pt->next == NULL) { // ��������һ���ڵ�
            printf("\n");
        }
    }
    for (int i = 0; i < page_size - current_total; i++) printf("\n"); // ����ҳ��
    printf("\n");
    printInMiddle_GBK("=============================\n", 38);
    printf("[��ǰ��������] ");
    if (strlen(search_kw) > 0) printf("ģ������=%s AND ", search_kw);
    printf("����ʽ=%s\n", SORT_METHOD_SEMESTER[sort_method]);
    printf("\n[��ǰѧ��] %s [��ǰѧУ����] %s\n\n", UTF8ToGBK(GLOBAL_semester), UTF8ToGBK(GLOBAL_school));
    printf("[��ʾ] ��%4d�����ݣ���ǰ��%3dҳ����%3dҳ\n�����ҷ���ǰ/��һҳ����/�·����л�ѡ�����ݣ�\n",
           total, page, max_page);
    printf("\n<A>����ѧ�� <Enter>�༭ѧ����Ϣ <D>ɾ��ѧ��\n<K>ѧ��ģ����ѯ <S>�����л���%s <Z>�޸�ѧУ����\n<X>�޸ĵ�ǰѧ��Ϊѡ���� <Esc>�������˵�\n",
           SORT_METHOD_SEMESTER[sort_method + 1 > 1 ? 0 : sort_method + 1]);

    if (selectedRow == NULL) {
        if (strlen(search_kw)) {
            printErrorData("û�в�ѯ������������ѧ��");
            strcpy(search_kw, "");
            goto Semester_GetAndDisplay;
        } else {
            printf("����ѧ�ڣ����ǹ���Ա���Ƿ��½�ѧ�ڣ�(Y)");
            int ch = getch();
            if (ch == 'Y' || ch == 'y') {
                editSemester(NULL);
                goto Semester_GetAndDisplay;
            } else {
                goto GC_Collect;
            }
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
                    if (selectedRow->next == NULL) selectedRow = semester_data_list->head;
                    else selectedRow = selectedRow->next;
                    goto Semester_Refresh;
                case 72: // ��
                    if (selectedRow->prev == NULL) selectedRow = semester_data_list->foot;
                    else selectedRow = selectedRow->prev;
                    goto Semester_Refresh;
                case 75: // ��
                    page = (page > 1) ? (page - 1) : 1;
                    selectedRow = 0;
                    goto Semester_GetAndDisplay;
                case 77: // ��
                    page = (page < max_page) ? (page + 1) : (max_page);
                    selectedRow = 0;
                    goto Semester_GetAndDisplay;
                default:
                    break;
            }
            break;
        case 's': // �޸�����˳��
        case 'S':
            selectedRow = 0;
            sort_method++;
            if (sort_method > 1) sort_method = 0;
            goto Semester_GetAndDisplay;
        case 'k': // �����ؼ���
        case 'K':
            printf("\n[ģ������] ������ؼ��ʣ���\"Enter\"����������");
            gets_safe(search_kw, 35);
            fflush(stdin);
            selectedRow = 0;
            goto Semester_GetAndDisplay;
        case 'A':
        case 'a': // �½�ѧ��
            editSemester(NULL);
            goto Semester_GetAndDisplay;
        case 'D':
        case 'd': // ɾ��ѧ��
        {
            SemesterLimit *pt = selectedRow->data;
            printf("\n[ȷ��ɾ��] ��ȷ��Ҫɾ��ѧ�� %s �𣿣���Ҫ������������%sȷ�ϣ�\n",
                   UTF8ToGBK(pt->semester), UTF8ToGBK(pt->semester));
            char input_char[31];
            gets_safe(input_char, 30);
            if (strcmp(input_char, pt->semester) != 0) {
                printf("ѧ�ڲ�һ�£���ȡ�����������������������\n");
                getch();
                goto Semester_GetAndDisplay;
            }
            cJSON *cancel_req_json = cJSON_CreateObject();
            cJSON_AddItemToObject(cancel_req_json, "semester", cJSON_CreateString(pt->semester));
            ResponseData cancel_resp = callBackend("semester.deleteSemester", cancel_req_json);
            cJSON_Delete(cancel_req_json);
            free(cancel_resp.raw_string);
            if (cancel_resp.status_code != 0) {
                printf("[ɾ��ʧ��] �����룺%d, %s���������������\n", cancel_resp.status_code,
                       UTF8ToGBK(findExceptionReason(&cancel_resp)));
                cJSON_Delete(cancel_resp.json);
                getch();
                goto Semester_GetAndDisplay;
            }
            printf("[ɾ���ɹ�] ѧ�� %s �ѱ�ɾ�������������������\n", UTF8ToGBK(pt->semester));
            getch();
            goto Semester_GetAndDisplay;
        }
        case 13: // �༭ѧ��
            editSemester((SemesterLimit *) selectedRow->data);
            goto Semester_GetAndDisplay;
        case 'x':
        case 'X': // �޸�ѧ��Ϊѡ����
        {
            cJSON *req = cJSON_CreateObject();
            cJSON_AddItemToObject(req, "semester", cJSON_CreateString(((SemesterLimit *) selectedRow->data)->semester));
            struct responseData x_resp = callBackend("semester.changeSemester", req);
            cJSON_Delete(req);
            free(x_resp.raw_string);
            if (x_resp.status_code != 0) {
                printf("[�޸�ѧ��ʧ��] �����룺%d, %s���������������\n", x_resp.status_code, UTF8ToGBK(findExceptionReason(&x_resp)));
            } else {
                printf("[�޸�ѧ�ڳɹ�] ��ǰѧ���ѱ�����Ϊ%s���������������\n", UTF8ToGBK(((SemesterLimit *) selectedRow->data)->semester));
                strcpy(GLOBAL_semester, ((SemesterLimit *) selectedRow->data)->semester);
            }
            cJSON_Delete(x_resp.json);
            getch();
            goto Semester_GetAndDisplay;
        }
        case 'z':
        case 'Z': // �޸�ѧУ����
        {
            char school_name[100];
            if (inputStringWithRegexCheck("[�޸�У��] ������У��������Ϊ3-50�ַ�������ĸ�����֡����ĺͿո���ɵ��ַ�����\n", SCHOOL_PATTERN, school_name,
                                          99) == -1)
                goto Semester_GetAndDisplay;
            cJSON *req = cJSON_CreateObject();
            cJSON_AddItemToObject(req, "school_name",
                                  cJSON_CreateString(school_name));
            struct responseData x_resp = callBackend("semester.changeSchoolName", req);
            cJSON_Delete(req);
            free(x_resp.raw_string);
            if (x_resp.status_code != 0) {
                printf("[�޸�У��ʧ��] �����룺%d, %s���������������\n", x_resp.status_code, UTF8ToGBK(findExceptionReason(&x_resp)));
            } else {
                printf("[�޸�У���ɹ�] ��ǰУ���ѱ�����Ϊ%s���������������\n", UTF8ToGBK(school_name));
                strcpy(GLOBAL_school, school_name);
            }
            cJSON_Delete(x_resp.json);
            getch();
            goto Semester_GetAndDisplay;
        }
        case 27:
            goto GC_Collect;
        default:
            break;
    }
    goto GetKey;

    GC_Collect:
    linkListObject_Delete(semester_data_list, 1);
    free(semester_data_list);
    semester_data_list = NULL;
}
