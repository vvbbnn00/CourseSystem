//
// Created by admin on 2022/6/26.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "course.h"
#include "cJSON.h"
#include "socket.h"
#include "global.h"
#include <Windows.h>
#include <conio.h>
#include "sysinfo.h"
#include "string_ext.h"
#include "link_list_object.h"
#include <math.h>
#include "simple_string_hash_list_obj.h"
#include "ui_gbk.h"

const char *SORT_METHOD_COURSE[3] = {"Ĭ������", "ѧ�ֽ���", "ѧ������"}; // ��ʾ����˳��
const char *NUMBER_CAPITAL[10] = {"��", "һ", "��", "��", "��", "��", "��", "��", "��", "��"};
const char *LECTURE_TYPE[4] = {"����", "ѡ��", "��ѡ", "����"};


/**
 * ��ȡ�γ�״̬
 *
 * @param status �γ�״̬
 * @return
 */
char *getCourseStatus(int status) {
    switch (status) {
        case 0:
            return "��ѡ";
        case 1:
            return "��ѡ";
        case 2:
            return "����";
        default:
            return "δ֪";
    }
}


/**
 * ��ӡ�γ̰���
 *
 * @param schedule
 * @return
 */
char *printSchedule(int schedule[7][13]) {
    char *final_str = (char *) calloc(2000, sizeof(char));
    if (final_str == NULL) return NULL;
    for (int i = 0; i < 6; i++) {
        char has_course = 0;
        char schedule_str[100] = "";
        char t_str[30] = "";
        char tt[10] = "";
        for (int j = 1; j <= 12; j++) {
            if (schedule[i][j]) {
                if (has_course) strcat(schedule_str, ",");
                has_course = 1;
                sprintf(tt, "%d", j);
                strcat(schedule_str, tt);
            }
        }
        if (has_course) {
            sprintf(t_str, "��%s:", NUMBER_CAPITAL[i + 1]);
            strcat(final_str, t_str);
            strcat(final_str, schedule_str);
            strcat(final_str, "��;");
        }
    }
    return final_str;
}


/**
 * ��ȡÿ����ѧʱ
 *
 * @param schedule
 * @return
 */
int getTotalWeekHour(int schedule[][13]) {
    int ans = 0;
    for (int i = 0; i < 7; i++) {
        for (int j = 1; j <= 12; j++) {
            if (schedule[i][j]) {
                ans++;
            }
        }
    }
    return ans;
}


/**
 * ��ӡ�γ���Ϣ
 * @param selected_course
 */
void printCourseData(Course *selected_course) {
    char *t = UTF8ToGBK(selected_course->title);
    printf("\n\t===== �γ���Ϣ��%s =====\n\n", t);
    free(t);
    t = UTF8ToGBK(selected_course->course_id);
    printf("\t��  ��ID��%s\n", t);
    free(t);
    t = UTF8ToGBK(selected_course->title);
    printf("\t�γ����ƣ�%s\n", t);
    free(t);
    t = UTF8ToGBK(selected_course->description);
    printf("\t�γ̼�飺%s\n", t);
    free(t);
    printf("\t�γ����ʣ�%s\n", LECTURE_TYPE[selected_course->type]);
    t = UTF8ToGBK(selected_course->semester);
    printf("\t����ѧ�ڣ�%s\n", t);
    free(t);
    printf("\t�ڿ���������%d��~��%d��\n", selected_course->week_start, selected_course->week_end);
    char *courseArrangeStr = printSchedule(selected_course->schedule);
    if (courseArrangeStr != NULL) {
        printf("\t�ڿΰ��ţ�%s\n", courseArrangeStr);
        free(courseArrangeStr);
    }
    char *t1 = UTF8ToGBK(selected_course->teacher.name), *t2 = UTF8ToGBK(selected_course->teacher.uid);
    printf("\t�ڿν�ʦ��%s(UID:%s)\n", t1, t2);
    free(t1);
    free(t2);
    printf("\t�γ�ѧʱ��%dѧʱ\n",
           getTotalWeekHour(selected_course->schedule) * (selected_course->week_end - selected_course->week_start + 1));
    printf("\t�γ�ѧ�֣�%.2f\n", selected_course->points);
}

/**
 * �����γ����ݣ�����ָ���Ӧ�ṹ��
 *
 * @param json
 * @param _dest
 * @return 0 �ɹ� ���� ʧ��
 */
int parseCourseData(cJSON *json, Course *_dest) {
    if (jsonParseInteger(json, "type", &_dest->type) ||
        jsonParseInteger(json, "week_start", &_dest->week_start) ||
        jsonParseInteger(json, "week_end", &_dest->week_end) ||
        jsonParseInteger(json, "max_members", &_dest->max_members) ||
        jsonParseInteger(json, "current_members", &_dest->current_members) ||
        jsonParseFloat(json, "points", &_dest->points)) {
        printErrorData("�γ���Ϣ���ݽ���ʧ�ܣ��뷵�����ԡ�");
        return -1;
    }
    jsonParseString(json, "course_id", _dest->course_id);
    jsonParseString(json, "title", _dest->title);
    jsonParseString(json, "description", _dest->description);
    jsonParseString(json, "semester", _dest->semester);

    cJSON *teacher_data = cJSON_GetObjectItem(json, "teacher");
    jsonParseString(teacher_data, "uid", _dest->teacher.uid);
    jsonParseString(teacher_data, "name", _dest->teacher.name);

    cJSON *schedule_dict = cJSON_GetObjectItem(json, "schedule");
    memset(_dest->schedule, 0, sizeof(_dest->schedule));
    for (int schedule = 1; schedule <= 7; schedule++) {
        char str[2];
        sprintf(str, "%d", schedule);
        cJSON *array_item = cJSON_GetObjectItem(schedule_dict, str);
        int array_length = cJSON_GetArraySize(array_item);
        for (int j = 0; j < array_length; j++) {
            int lecture_key = -1;
            if (jsonArrayParseInteger(array_item, j, &lecture_key)) {
                printErrorData("ҳ����ϸ����(�ڴΰ���)����ʧ�ܣ��뷵�����ԡ�");
            }
            _dest->schedule[schedule - 1][lecture_key] = 1;
        }
    }
    return 0;
}

/**
 * ִ��ѧ��ѡ������
 */
void printStudentCourseSelection() {
    system("chcp 936>nul & cls & MODE CON COLS=110 LINES=50");
    HANDLE windowHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    char search_kw[36] = "";
    double max_score = 0, current_score = 0; // ��ѧ������ѡѧ��, ��ǰ��ѡѧ��
    int sort_method = 0; // ���򷽷� 0 - Ĭ������ 1 - ѧ�ֽ��� 2 - ѧ������
    int total, page = 1, max_page, page_size = 10, current_total;

    LinkList_Node *selectedRow = NULL; // ��ѡ�е���
    cJSON *req_json = NULL;
    LinkList_Object *course_data_list = NULL;

    GetCourseAndDisplay:
    // ... �������
    printf("[��ʾ] �������������...\n");
    req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "course_selection", cJSON_CreateNumber(1)); // ѡ�γ�������Ҫ�����������
    cJSON_AddItemToObject(req_json, "page", cJSON_CreateNumber(page));
    cJSON_AddItemToObject(req_json, "size", cJSON_CreateNumber(page_size));
    cJSON_AddItemToObject(req_json, "semester", cJSON_CreateString(GLOBAL_semester));
    cJSON_AddItemToObject(req_json, "kw", cJSON_CreateString(GBKToUTF8(search_kw)));
    cJSON_AddItemToObject(req_json, "sort_method", cJSON_CreateNumber(sort_method));
    ResponseData resp = callBackend("course.getCourseList", req_json);
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
        jsonParseInteger(ret_json, "page_size", &page_size) ||
        jsonParseFloat(ret_json, "current_score", &current_score) ||
        jsonParseFloat(ret_json, "max_score", &max_score)) {
        printErrorData("ҳ��������ݽ���ʧ�ܣ��뷵�����ԡ�");
        if (resp.json != NULL) cJSON_Delete(resp.json);
        if (resp.raw_string != NULL) free(resp.raw_string);
        return;
    }

    // �����γ�����
    course_data_list = linkListObject_Init(); // �����ʼ��
    cJSON *data = cJSON_GetObjectItem(ret_json, "data");
    for (int i = 0; i < current_total; i++) {
        cJSON *row_data = cJSON_GetArrayItem(data, i);
        struct studentCourseSelection *course_data_pt = calloc(1, sizeof(struct studentCourseSelection));
        if (parseCourseData(row_data, &course_data_pt->course)) return;
        if (jsonParseInteger(row_data, "status", &course_data_pt->status) ||
            jsonParseLong(row_data, "selection_time", &course_data_pt->selection_time)) {
            printErrorData("ҳ����ϸ���ݽ���ʧ�ܣ��뷵�����ԡ�");
        }
        jsonParseString(row_data, "locked_reason", course_data_pt->locked_reason);
        LinkList_Node *node = linkListObject_Append(course_data_list, course_data_pt); // ����β��׷��Ԫ��
        if (node == NULL) {
            printErrorData("�ڴ����");
        }
        if (i == 0) selectedRow = node;
    }
    cJSON_Delete(ret_json);
    free(resp.raw_string);

    Refresh:

    system("cls");
    printf("\n");
    ui_printHeader_GBK(102);
    printf("\n");
    printInMiddle_GBK("======= �γ̡�ѧ��ѡ�� =======\n", 104);
    printf("%-16s%-35s%-35s%-10s%-12s\n", "ѡ��״̬", "�γ�ID", "�γ�����", "�γ�ѧ��", "�ڿν�ʦ");
    printf("--------------------------------------------------------------------------------------------------------\n");
    for (LinkList_Node *pt = course_data_list->head; pt != NULL; pt = pt->next) {
        struct studentCourseSelection *tmp = pt->data;
        if (pt == selectedRow) SetConsoleTextAttribute(windowHandle, 0x70);
        else {
            switch (tmp->status) {
                case 1: // ��ѡ
                    SetConsoleTextAttribute(windowHandle, 0x0e);
                    break;
                case 2: // ����
                    SetConsoleTextAttribute(windowHandle, 0x08);
                    break;
                default:
                    break;
            }
        }
        printf("[%4s]%3d/%-3d   %-35s%-35s%-10.2f%-12s\n",
               getCourseStatus(tmp->status),
               tmp->course.current_members,
               tmp->course.max_members,
               UTF8ToGBK(tmp->course.course_id),
               UTF8ToGBK(tmp->course.title),
               tmp->course.points,
               UTF8ToGBK(tmp->course.teacher.name));
        SetConsoleTextAttribute(windowHandle, 0x07);
        if (pt->next == NULL) { // ��������һ���ڵ�
            printf("\n");
        }
    }
    for (int i = 0; i < page_size - current_total; i++) printf("\n"); // ����ҳ��
    printf("\n");
    printInMiddle_GBK("=============================\n", 104);
    printf("[��ǰ��������] ");
    if (strlen(search_kw) > 0) printf("ģ������=%s AND ", search_kw);
    printf("����ʽ=%s", SORT_METHOD_COURSE[sort_method]);
    printf("  [ѡ�θſ�] %sѧ�ڣ���ѡ%.2fѧ�֣�����ѡ%.2fѧ��\n\n", UTF8ToGBK(GLOBAL_semester), current_score, max_score);
    printf("[��ʾ] ��%4d�����ݣ���ǰ��%3dҳ����%3dҳ���������ǰһҳ���ҷ��������һҳ����/�·�������л�ѡ�����ݣ�\n",
           total, page, max_page);
    printf("\n\t   <Enter>ѡ��/��ѡ <K>�γ�ģ����ѯ <S>�����л���%s <Esc>�������˵�\n",
           SORT_METHOD_COURSE[sort_method + 1 > 2 ? 0 : sort_method + 1]);

    if (selectedRow == NULL) {
        printErrorData("���޿�ѡ�γ�");
//        getch();
        if (strlen(search_kw) == 0)
            goto GC_Collect;
        else {
            strcpy(search_kw, "");
            goto GetCourseAndDisplay;
        }
    }

    struct studentCourseSelection *selected_selection = selectedRow->data;
    Course *selected_course = &selected_selection->course;

    printCourseData(selected_course);
    switch (selected_selection->status) {
        case 1: // ��ѡ
            SetConsoleTextAttribute(windowHandle, 0x0e);
            break;
        case 2: // ����
            SetConsoleTextAttribute(windowHandle, 0x08);
            break;
        default:
            break;
    }
    printf("\t״  ̬��  %s[%d/%d��]%s%s%s%s\n",
           getCourseStatus(selected_selection->status),
           selected_course->current_members,
           selected_course->max_members,
           strlen(selected_selection->locked_reason) > 0 ? " - ����ѡԭ��" : "",
           UTF8ToGBK(selected_selection->locked_reason),
           selected_selection->selection_time > 0 ? " - ѡ��ʱ�䣺" : "",
           selected_selection->selection_time > 0 ?
           getFormatTimeString(selected_selection->selection_time) : "");
    SetConsoleTextAttribute(windowHandle, 0x07);

    int keyboard_press;

    GetKey:
    keyboard_press = _getch();
    cJSON *xk_json = NULL;
    switch (keyboard_press) {
        case 224:
            keyboard_press = _getch();
            switch (keyboard_press) {
                case 80: // ��
                    if (selectedRow->next == NULL) selectedRow = course_data_list->head;
                    else selectedRow = selectedRow->next;
                    goto Refresh;
                case 72: // ��
                    if (selectedRow->prev == NULL) selectedRow = course_data_list->foot;
                    else selectedRow = selectedRow->prev;
                    goto Refresh;
                case 75: // ��
                    page = (page > 1) ? (page - 1) : 1;
                    selectedRow = 0;
                    goto GetCourseAndDisplay;
                case 77: // ��
                    page = (page < max_page) ? (page + 1) : (max_page);
                    selectedRow = 0;
                    goto GetCourseAndDisplay;
                default:
                    break;
            }
            break;
        case 's': // �޸�����˳��
        case 'S':
            selectedRow = 0;
            sort_method++;
            if (sort_method > 2) sort_method = 0;
            goto GetCourseAndDisplay;
        case 'k': // �����ؼ���
        case 'K':
            printf("\n[ģ������]������ؼ��ʣ���\"Enter\"����������");
            gets_safe(search_kw, 35);
            fflush(stdin);
            selectedRow = 0;
            goto GetCourseAndDisplay;
        case 13: // ѡ��ѡ��
            printf("[��ʾ] �������������...");
            xk_json = cJSON_CreateObject();
            cJSON_AddItemToObject(xk_json, "course_id", cJSON_CreateString(selected_course->course_id));
            if (selected_selection->status == 0) {
                if (selected_course->points + current_score > max_score) {
                    printf("\n[ѡ��ʧ��] �γ�:%s(%s)ѡ��ʧ�ܣ�����ѧ������Ϊ%.2f��\n",
                           UTF8ToGBK(selected_course->title),
                           UTF8ToGBK(selected_course->course_id),
                           max_score);
                    goto After;
                }
                ResponseData xk_resp = callBackend("course.select", xk_json);
                free(xk_resp.raw_string);
                if (xk_resp.status_code == 0) {
                    printf("\n[ѡ�γɹ�] �γ�:%s(%s)��ѡ��ɹ������������������\n",
                           UTF8ToGBK(selected_course->title),
                           UTF8ToGBK(selected_course->course_id));
                } else {
                    printf("\n[ѡ��ʧ��] �γ�:%s(%s)ѡ��ʧ�ܣ������룺%d��, %s��\n",
                           UTF8ToGBK(selected_course->title),
                           UTF8ToGBK(selected_course->course_id),
                           xk_resp.status_code,
                           UTF8ToGBK(findExceptionReason(&xk_resp)));
                }
                cJSON_Delete(xk_resp.json);
            } else {
                if (selected_selection->status == 1) {
                    ResponseData xk_resp = callBackend("course.cancel", xk_json);
                    free(xk_resp.raw_string);
                    if (xk_resp.status_code == 0) {
                        printf("\n[��ѡ�ɹ�] �γ�:%s(%s)�ѳɹ���ѡ�����������������\n",
                               UTF8ToGBK(selected_course->title),
                               UTF8ToGBK(selected_course->course_id));
                    } else {
                        printf("\n[��ѡʧ��] �γ�:%s(%s)��ѡʧ�ܣ������룺%d��, %s��\n",
                               UTF8ToGBK(selected_course->title),
                               UTF8ToGBK(selected_course->course_id),
                               xk_resp.status_code,
                               UTF8ToGBK(findExceptionReason(&xk_resp)));
                    }
                    cJSON_Delete(xk_resp.json);
                } else
                    printf("\n[ѡ��ʧ��] ���޷�ѡ��ÿγ̣�%s���������������\n",
                           UTF8ToGBK(selected_selection->locked_reason));
            }
        After:
            cJSON_Delete(xk_json);
            getch();
            goto GetCourseAndDisplay;
        case 27:
            goto GC_Collect;
        default:
            break;
    }
    goto GetKey;

    GC_Collect:
    linkListObject_Delete(course_data_list, 1);
    free(course_data_list);
    course_data_list = NULL;
}

/**
 * ����α��ļ����У���������̨��
 *
 * @param _stream
 * @param scheduleList
 */
void printTableToStream(FILE *_stream, LinkList_Object scheduleList[7][13]) {
    for (int week_num = 0; week_num < 7; week_num++) {
        char has_course = 0; // ��ǰ�����Ƿ��п�
        fprintf(_stream, "[����%s]\n\n", NUMBER_CAPITAL[week_num + 1]);
        for (int course_num = 1; course_num <= 12; course_num++) {
            LinkList_Node *head = scheduleList[week_num][course_num].head;
            if (head == NULL) continue; // ��ʱ��û�п�
            has_course = 1;
            for (LinkList_Node *pt = head; pt != NULL; pt = pt->next) {
                Course *c_data = pt->data;
                fprintf(_stream, "  <��");
                for (int i = course_num; i <= 12; i++) {
                    if (!c_data->schedule[week_num][i]) break;
                    fprintf(_stream, " %d", i);
                }
                fprintf(_stream, " ��> %s\n", UTF8ToGBK(c_data->title));
                fprintf(_stream, "    �ڿν�ʦ��%s���ڿ��ܣ���%d������%d�ܣ�ѧ�֣�%.2f\n",
                        UTF8ToGBK(c_data->teacher.name),
                        c_data->week_start,
                        c_data->week_end,
                        c_data->points);
            }
        }
        if (!has_course) fprintf(_stream, "  �޿γ�\n");
        fprintf(_stream, "\n");
    }
}


/**
 * ���ѧ���α���չʾ�ڿ���̨
 */
void printStudentLectureTable() {
    system("chcp 936>nul & cls & MODE CON COLS=70 LINES=9001"); // ��������һ��Ҫ�󣬲�Ȼ���ݻᱻˢ��

    int total; // �ܽ������
    double score_total; // ��ѧ��
    LinkList_Object scheduleList[7][13] = {0}; // ������Ŀα����ݷ��������Ӧ�ڴε���Ϣ[7]��ʾ��һ(0)������(6)��[13]��ʾ��һ�ڿ�(1)����ʮ���ڿ�(12)

    cJSON *req_json = NULL;

    // ... �������
    printf("[��ʾ] �������������...\n");
    req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "semester", cJSON_CreateString(GLOBAL_semester));
    ResponseData resp = callBackend("course.getStuCourseTable", req_json);
    cJSON_Delete(req_json);
    cJSON *ret_json = resp.json;
    if (resp.status_code != 0) {
        printErrorData(UTF8ToGBK(findExceptionReason(&resp)));
        if (ret_json != NULL) cJSON_Delete(ret_json);
        if (resp.raw_string != NULL) free(resp.raw_string);
        return;
    }

    // ����ҳ��������ݣ�ҳ���
    if (jsonParseInteger(ret_json, "total", &total) ||
        jsonParseInteger(ret_json, "total", &total) ||
        jsonParseFloat(ret_json, "score_total", &score_total)) {
        printErrorData("ҳ��������ݽ���ʧ�ܣ��뷵�����ԡ�");
        if (resp.json != NULL) cJSON_Delete(resp.json);
        if (resp.raw_string != NULL) free(resp.raw_string);
        return;
    }

    // �����γ�����
    cJSON *data = cJSON_GetObjectItem(ret_json, "data");
    for (int i = 0; i < total; i++) {
        cJSON *row_data = cJSON_GetArrayItem(data, i);
        Course *course_data_pt = alloca(sizeof(Course));
        if (parseCourseData(row_data, course_data_pt)) return;
        for (int week = 0; week < 7; week++) {
            int continuous = 0; // �ж��Ƿ��������Ŀγ̰��ţ������ ������һ�𣬲��ظ�����ڿα���
            for (int seq = 1; seq <= 12; seq++) {
                if (course_data_pt->schedule[week][seq]) {  // ��ʱ���п�
                    if (!continuous) {
                        linkListObject_Append(&scheduleList[week][seq], course_data_pt); // ���γ̼����ȥ
                        continuous = 1; // �ظ���flag����
                    }
                } else {
                    continuous = 0; // ����һ�����ظ��ģ���flag����
                }
            }
        }
    }

    Refresh:

    system("cls");
    printf("\n");
    ui_printHeader_GBK(58);
    printf("\n");
    printInMiddle_GBK("======= �γ̡���ǰѧ�ڿα� =======\n", 60);
    printTableToStream(stdout, scheduleList);
    printf("------------------------------------------------------------\n");
    printf("\n    [ѧ������] %s  [��ǰѧ��] %s  [��ѡѧ��] %.2f\n",
           UTF8ToGBK(GLOBAL_user_info.name),
           UTF8ToGBK(GLOBAL_semester),
           score_total);
    printf("\n\t <E>�����α��ļ��� <Esc>�������˵�\n\n");

    int keyboard_press;

    GetKey:
    keyboard_press = _getch();
    switch (keyboard_press) {
        case 'e': // �����α�
        case 'E': {
            time_t raw_time;
            time(&raw_time);
            char *timestamp = getFormatTimeString_("%Y%m%d%H%M%S", raw_time);
            char *name = UTF8ToGBK(GLOBAL_user_info.name);
            char *semester = UTF8ToGBK(GLOBAL_semester);
            unsigned long long len = strlen(timestamp) +
                                     strlen(name) +
                                     strlen(semester) +
                                     strlen("��ѧ�ڿα�_.txt") + 1;
            char file_name[len];
            memset(file_name, 0, len);
            sprintf(file_name, "%s��%sѧ�ڿα�_%s.txt", name, GLOBAL_semester, timestamp);
            free(name);
            free(semester);
            free(timestamp);
            FILE *file = fopen(file_name, "w");
            printf("[��ʾ] ���ڽ��α�������%s...\n", file_name);
            if (file == NULL) {
                printf("[����ʧ��] �޷������ļ�\"%s\"�������Ƿ��ж�дȨ�ޣ��������������\n", file_name);
                goto Refresh;
            }
            printTableToStream(file, scheduleList);
            fclose(file);
            printf("[�����ɹ�] �α��ѵ�������%s���������������\n", file_name);
            getch();
            goto Refresh;
        }
        case 27:
            goto GC_Collect;
        default:
            break;
    }
    goto GetKey;

    GC_Collect:
    for (int i = 0; i < 7; i++)
        for (int j = 1; j <= 12; j++) linkListObject_Delete(&scheduleList[i][j], 0);
}


/**
 * ���ȫУ�α�����Ա�汾��
 * @param scene ���� 0 - �鿴ȫУ�α�����Ա�ɹ��� 1 - ��ʦ������ֻ��ʾ�Լ��̵Ŀγ̣���������
 */
void printAllCourses(int scene) {
    HANDLE windowHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    char search_kw[36] = "", search_semester[36] = "";
    int sort_method = 0; // ���򷽷� 0 - Ĭ������ 1 - ѧ�ֽ��� 2 - ѧ������
    int total, page = 1, max_page, page_size = 15, current_total;

    LinkList_Node *selectedRow = NULL; // ��ѡ�е���
    cJSON *req_json = NULL;
    LinkList_Object *course_data_list = NULL;

    GetCourseAndDisplay:
    system("chcp 936>nul & cls & MODE CON COLS=130 LINES=55");
    // ... �������
    printf("[��ʾ] �������������...\n");
    req_json = cJSON_CreateObject();
    if (scene == 1) {
        cJSON_AddItemToObject(req_json, "manage", cJSON_CreateNumber(1));
    }
    cJSON_AddItemToObject(req_json, "page", cJSON_CreateNumber(page));
    cJSON_AddItemToObject(req_json, "size", cJSON_CreateNumber(page_size));
    cJSON_AddItemToObject(req_json, "semester", cJSON_CreateString(GBKToUTF8(search_semester)));
    cJSON_AddItemToObject(req_json, "kw", cJSON_CreateString(GBKToUTF8(search_kw)));
    cJSON_AddItemToObject(req_json, "sort_method", cJSON_CreateNumber(sort_method));
    ResponseData resp = callBackend("course.getCourseList", req_json);
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

    // �����γ�����
    course_data_list = linkListObject_Init(); // �����ʼ��
    cJSON *data = cJSON_GetObjectItem(ret_json, "data");
    for (int i = 0; i < current_total; i++) {
        cJSON *row_data = cJSON_GetArrayItem(data, i);
        Course *course_data_pt = calloc(1, sizeof(Course));
        if (parseCourseData(row_data, course_data_pt)) return;
        LinkList_Node *node = linkListObject_Append(course_data_list, course_data_pt); // ����β��׷��Ԫ��
        if (node == NULL) {
            printErrorData("�ڴ����");
        }
        if (i == 0) selectedRow = node;
    }
    cJSON_Delete(ret_json);
    free(resp.raw_string);

    Refresh:

    system("cls");
    printf("\n");
    ui_printHeader_GBK(120);
    printf("\n");
    printInMiddle_GBK("======= �γ̡�ȫУ�α�һ�� =======\n", 122);
    printf("%-35s%-35s%-20s%-10s%-10s%-12s\n", "�γ�ID", "�γ�����", "�γ�ѧ��", "�γ�ѧ��", "�γ�����", "�ڿν�ʦ");
    printf("--------------------------------------------------------------------------------------------------------------------------\n");
    for (LinkList_Node *pt = course_data_list->head; pt != NULL; pt = pt->next) {
        Course *tmp = pt->data;
        if (pt == selectedRow) SetConsoleTextAttribute(windowHandle, 0x70);
        printf("%-35s%-35s%-20s%-10.2f%-10s%-12s\n",
               UTF8ToGBK(tmp->course_id),
               UTF8ToGBK(tmp->title),
               UTF8ToGBK(tmp->semester),
               tmp->points,
               LECTURE_TYPE[tmp->type],
               UTF8ToGBK(tmp->teacher.name));
        SetConsoleTextAttribute(windowHandle, 0x07);
        if (pt->next == NULL) { // ��������һ���ڵ�
            printf("\n");
        }
    }
    for (int i = 0; i < page_size - current_total; i++) printf("\n"); // ����ҳ��
    printf("\n");
    printInMiddle_GBK("=============================\n", 122);
    printf("[��ǰ��������] ");
    if (strlen(search_kw) > 0) printf("ģ������=%s AND ", search_kw);
    if (strlen(search_semester) > 0) printf("ָ��ѧ��=%s AND ", search_semester);
    printf("����ʽ=%s\n", SORT_METHOD_COURSE[sort_method]);
    printf("[��ʾ] ��%4d�����ݣ���ǰ��%3dҳ����%3dҳ���������ǰһҳ���ҷ��������һҳ����/�·�������л�ѡ�����ݣ�\n",
           total, page, max_page);
    printf("\n");
    if (GLOBAL_user_info.role == 2 || (GLOBAL_user_info.role == 1 && scene == 1)) { // ����Ա�ɱ༭�γ�
        printf("<A>����/�½��γ� <Enter>�༭�γ� <P>�鿴ѧ������ <D>ɾ���γ�");
    } else {
        printf("\t  ");
    }
    printf(" <K>�γ�ģ����ѯ <F>ָ���γ�ѧ�ڲ�ѯ <S>�����л���%s",
           SORT_METHOD_COURSE[sort_method + 1 > 2 ? 0 : sort_method + 1]);
    printf(" <Esc>�������˵�\n");

    if (selectedRow == NULL) {
        if (strlen(search_kw) || strlen(search_semester)) {
            printErrorData("û�в�ѯ�����������Ŀγ�");
            strcpy(search_semester, "");
            strcpy(search_kw, "");
            goto GetCourseAndDisplay;
        } else {
            if ((GLOBAL_user_info.role == 2 || (GLOBAL_user_info.role == 1 && scene == 1))) { // ����Ա�ɱ༭�γ�
                printf("���޿γ̣��Ƿ����������γ̣�(Y)");
                int ch = getch();
                if (ch == 'Y' || ch == 'y') {
                    editCourse(NULL);
                }
            } else {
                printErrorData("���޿γ�");
            }
            goto GC_Collect;
        }
    }

    printCourseData(selectedRow->data);

    int keyboard_press;

    GetKey:
    keyboard_press = _getch();
    switch (keyboard_press) {
        case 224:
            keyboard_press = _getch();
            switch (keyboard_press) {
                case 80: // ��
                    if (selectedRow->next == NULL) selectedRow = course_data_list->head;
                    else selectedRow = selectedRow->next;
                    goto Refresh;
                case 72: // ��
                    if (selectedRow->prev == NULL) selectedRow = course_data_list->foot;
                    else selectedRow = selectedRow->prev;
                    goto Refresh;
                case 75: // ��
                    page = (page > 1) ? (page - 1) : 1;
                    selectedRow = 0;
                    goto GetCourseAndDisplay;
                case 77: // ��
                    page = (page < max_page) ? (page + 1) : (max_page);
                    selectedRow = 0;
                    goto GetCourseAndDisplay;
                default:
                    break;
            }
            break;
        case 's': // �޸�����˳��
        case 'S':
            selectedRow = 0;
            sort_method++;
            if (sort_method > 2) sort_method = 0;
            goto GetCourseAndDisplay;
        case 'k': // �����ؼ���
        case 'K':
            printf("\n[ģ������]������ؼ��ʣ���\"Enter\"����������");
            gets_safe(search_kw, 35);
            fflush(stdin);
            selectedRow = 0;
            goto GetCourseAndDisplay;
        case 'f': // ɸѡѧ��
        case 'F':
            printf("\n[ָ����ѯѧ��]������ѧ�ڣ���\"Enter\"����������");
            gets_safe(search_semester, 35);
            fflush(stdin);
            selectedRow = 0;
            goto GetCourseAndDisplay;
        case 'A':
        case 'a':
            if (!(GLOBAL_user_info.role == 2 || (GLOBAL_user_info.role == 1 && scene == 1))) break; // Ȩ���ж�
            editCourse(NULL);
            goto GetCourseAndDisplay;
        case 'D':
        case 'd': // ɾ���γ�
            if (!(GLOBAL_user_info.role == 2 || (GLOBAL_user_info.role == 1 && scene == 1))) break; // Ȩ���ж�
            {
                Course *pt = selectedRow->data;
                printf("\n[ȷ��ɾ��] ��ȷ��Ҫɾ���γ� %s(%s) �𣿣�ɾ���γ̺󣬸ÿγ����е�ѧ��ѡ�μ�¼������գ���Ҫ������������ÿγ�ID:%sȷ�ϣ�\n",
                       UTF8ToGBK(pt->title),
                       UTF8ToGBK(pt->course_id), UTF8ToGBK(pt->course_id));
                char input_char[33];
                gets_safe(input_char, 32);
                if (strcmp(input_char, pt->course_id) != 0) {
                    printf("�γ�ID��һ�£���ȡ�����������������������\n");
                    getch();
                    goto GetCourseAndDisplay;
                }
                cJSON *cancel_req_json = cJSON_CreateObject();
                cJSON_AddItemToObject(cancel_req_json, "course_id", cJSON_CreateString(pt->course_id));
                ResponseData cancel_resp = callBackend("course.deleteCourse", cancel_req_json);
                cJSON_Delete(cancel_req_json);
                free(cancel_resp.raw_string);
                if (cancel_resp.status_code != 0) {
                    printf("[ɾ��ʧ��] �����룺%d, %s���������������\n", cancel_resp.status_code,
                           UTF8ToGBK(findExceptionReason(&cancel_resp)));
                    cJSON_Delete(cancel_resp.json);
                    getch();
                    goto GetCourseAndDisplay;
                }
                printf("[ɾ���ɹ�] �γ� %s(%s) �����ѡ�μ�¼�ѱ�ɾ�������������������\n", UTF8ToGBK(pt->title), UTF8ToGBK(pt->course_id));
                getch();
                goto GetCourseAndDisplay;
            }
        case 'p':
        case 'P':
            if (!(GLOBAL_user_info.role == 2 || (GLOBAL_user_info.role == 1 && scene == 1))) break; // Ȩ���ж�
            printStudentList((Course *) selectedRow->data);
            goto GetCourseAndDisplay;
        case 13: // �༭�γ�
            if (!(GLOBAL_user_info.role == 2 || (GLOBAL_user_info.role == 1 && scene == 1))) break; // Ȩ���ж�
            editCourse((Course *) selectedRow->data);
            goto GetCourseAndDisplay;
        case 27:
            goto GC_Collect;
        default:
            break;
    }
    goto GetKey;

    GC_Collect:
    linkListObject_Delete(course_data_list, 1);
    free(course_data_list);
    course_data_list = NULL;
}


/**
 * �����γ����ݣ�����ָ���Ӧ�ṹ��
 *
 * @param json
 * @param _dest
 * @return 0 �ɹ� ���� ʧ��
 */
int parseStudentData(cJSON *json, struct teacherCourseSelection *_dest) {
    if (jsonParseLong(json, "selection_time", &_dest->selection_time)) {
        printErrorData("�γ���Ϣ���ݽ���ʧ�ܣ��뷵�����ԡ�");
        return -1;
    }
    jsonParseString(json, "uid", _dest->student.uid);
    jsonParseString(json, "name", _dest->student.name);
    return 0;
}


/**
 * ����ѧ���б�
 * @param linkList
 * @param course
 * @return
 */
int exportStudentList(LinkList_Object *linkList, Course *course) {
    time_t raw_time;
    time(&raw_time);
    char *timestamp = getFormatTimeString_("%Y%m%d%H%M%S", raw_time);
    char *title = UTF8ToGBK(course->title);
    char *course_id = UTF8ToGBK(course->course_id);
    unsigned long long len = strlen(timestamp) +
                             strlen(title) +
                             strlen(course_id) +
                             strlen("()ѧ������_.csv") + 1;
    char file_name[len];
    memset(file_name, 0, len);
    sprintf(file_name, "%s(%s)ѧ������_%s.csv", title, course_id, timestamp);
    free(title);
    free(course_id);
    free(timestamp);
    FILE *file = fopen(file_name, "w");
    printf("[��ʾ] ���ڽ�ѧ��������������%s...\n", file_name);
    if (file == NULL) {
        printf("[����ʧ��] �޷������ļ�\"%s\"�������Ƿ��ж�дȨ�ޣ��������������\n", file_name);
        return -1;
    }
    fprintf(file, "���,����,ѧ��,ѡ��ʱ��\n");
    int counter = 1;
    for (LinkList_Node *pt = linkList->head; pt != NULL; pt = pt->next) {
        struct teacherCourseSelection *p = pt->data;
        fprintf(file, "%d,%s,%s,%s\n",
                counter,
                UTF8ToGBK(p->student.name),
                UTF8ToGBK(p->student.uid),
                getFormatTimeString(p->selection_time));
        counter++;
    }
    fclose(file);
    printf("[�����ɹ�] ѧ�������ѵ�������%s���������������\n", file_name);
    return 0;
}


/**
 * ��ʦ/����Ա��ӡѧ������
 */
void printStudentList(Course *courseData) {
    system("chcp 936>nul & cls & MODE CON COLS=80 LINES=55");
    HANDLE windowHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    int sort_method = 0; // ���򷽷� 0 - Ĭ������ 1 - ѧ�ֽ��� 2 - ѧ������
    int total, page = 1, page_size = 30, max_page;

    cJSON *req_json = NULL;
    LinkList_Object *student_list = NULL;
    LinkList_Node *selectedRow = NULL;

    Teacher_GetCourseAndDisplay:
    // ... �������
    printf("[��ʾ] �������������...\n");
    req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "course_id", cJSON_CreateString(courseData->course_id));
    ResponseData resp = callBackend("course.getStudentSelectionList", req_json);
    cJSON_Delete(req_json);
    cJSON *ret_json = resp.json;
    if (resp.status_code != 0) {
        printErrorData(UTF8ToGBK(findExceptionReason(&resp)));
        if (ret_json != NULL) cJSON_Delete(ret_json);
        if (resp.raw_string != NULL) free(resp.raw_string);
        return;
    }

    // ��������������
    if (jsonParseInteger(ret_json, "total", &total)) {
        printErrorData("ҳ��������ݽ���ʧ�ܣ��뷵�����ԡ�");
        if (resp.json != NULL) cJSON_Delete(resp.json);
        if (resp.raw_string != NULL) free(resp.raw_string);
        return;
    }
    max_page = (int) ceilf((float) total / (float) page_size);

    // ������������
    student_list = linkListObject_Init(); // �����ʼ��
    cJSON *data = cJSON_GetObjectItem(ret_json, "data");
    for (int i = 0; i < total; i++) {
        cJSON *row_data = cJSON_GetArrayItem(data, i);
        struct teacherCourseSelection *data_pt = calloc(1, sizeof(struct teacherCourseSelection));
        if (parseStudentData(row_data, data_pt)) return;
        LinkList_Node *node = linkListObject_Append(student_list, data_pt); // ����β��׷��Ԫ��
        if (node == NULL) {
            printErrorData("�ڴ����");
        }
        if (i == 0) selectedRow = node;
    }
    cJSON_Delete(ret_json);
    free(resp.raw_string);

    Teacher_Refresh:

    system("cls");
    printf("\n");
    ui_printHeader_GBK(69);
    printf("\n");
    printInMiddle_GBK("======= �γ̡��γ����� =======\n", 71);
    printf("%-6s%-15s%-20s%-30s\n", "���", "����", "ѧ��", "ѡ��ʱ��");
    printf("-----------------------------------------------------------------------\n");
    int counter = 0, current_total = 0;
    for (LinkList_Node *pt = student_list->head; pt != NULL; pt = pt->next) {
        counter++;
        if (ceilf((float) counter / (float) page_size) < (float) page) continue; // ��λ����ҳ�棬ѧ��������ҳ��ʾ
        if (ceilf((float) counter / (float) page_size) > (float) page) break; // һҳ��ʾ�꼴������ѭ��
        current_total++;
        if (selectedRow == NULL) selectedRow = pt;
        struct teacherCourseSelection *tmp = pt->data;
        if (pt == selectedRow) SetConsoleTextAttribute(windowHandle, 0x70);
        printf("%4d  %-15s%-20s%-30s\n",
               counter,
               UTF8ToGBK(tmp->student.name),
               UTF8ToGBK(tmp->student.uid),
               getFormatTimeString(tmp->selection_time));
        SetConsoleTextAttribute(windowHandle, 0x07);
        if (pt->next == NULL) { // ��������һ���ڵ�
            printf("\n");
        }
    }
    for (int i = 0; i < page_size - current_total - 1; i++) printf("\n"); // ����ҳ��
    printf("\n");
    printInMiddle_GBK("=============================\n", 71);
    printf("    [�γ�����] %s [�γ�ID] %s\n\n", UTF8ToGBK(courseData->title), courseData->course_id);
    printf("    [��ʾ] ��%4d�����ݣ���ǰ��%3dҳ����%3dҳ\n\n    ���������ǰһҳ���ҷ��������һҳ����/�·�������л�ѡ�����ݣ�\n",
           total, page, max_page);
    printf("\n  ");
    if (GLOBAL_user_info.role == 2) { // ����Ա�ɱ༭�γ�
        printf("<A>���ѧ�� <D>ȡ����ѧ��ѡ�� <I>��������ѧ��");
    } else {
        printf("\t");
    }
    printf(" <E>����ѧ������ <Esc>�������˵�\n");

    if (selectedRow == NULL) {
        printErrorData("��������");
        if (GLOBAL_user_info.role == 2) { // ����Ա�ɱ༭�γ�
            printf("���ǹ���Ա���Ƿ���ѧ��������(Y)");
            int ch = getch();
            if (ch == 'Y' || ch == 'y') {
                importStuCourseData();
            }
        }
        goto Teacher_GC_Collect;
    }
    int keyboard_press;

    Teacher_GetKey:
    keyboard_press = _getch();
    switch (keyboard_press) {
        case 224:
            keyboard_press = _getch();
            switch (keyboard_press) {
                case 80: // ��
                    if (selectedRow->next == NULL) selectedRow = student_list->head;
                    else selectedRow = selectedRow->next;
                    goto Teacher_Refresh;
                case 72: // ��
                    if (selectedRow->prev == NULL) selectedRow = student_list->foot;
                    else selectedRow = selectedRow->prev;
                    goto Teacher_Refresh;
                case 75: // ��
                    page = (page > 1) ? (page - 1) : 1;
                    selectedRow = 0;
                    goto Teacher_Refresh;
                case 77: // ��
                    page = (page < max_page) ? (page + 1) : (max_page);
                    selectedRow = 0;
                    goto Teacher_Refresh;
                default:
                    break;
            }
            break;
        case 'E':
        case 'e':
            exportStudentList(student_list, courseData);
            _getch();
            goto Teacher_Refresh;
        case 'I':
        case 'i': // ��������ѧ����Ϣ
            if (GLOBAL_user_info.role != 2) break; // �޷�����ѧ��
            importStuCourseData();
            goto Teacher_GetCourseAndDisplay;
        case 'A':
        case 'a': // ����ѡ��ѧ��
            if (GLOBAL_user_info.role != 2) break; // �޷�����ѧ��
            {
                char user_id[31] = {0};
                if (inputStringWithRegexCheck("[�γ�����ѧ��] ������ѧ��ѧ�ţ�������������ѧ����Ϣ����ʹ�á���������ѧ�������ܣ�\n",
                                              USER_PATTERN,
                                              user_id,
                                              30) == -1)
                    goto Teacher_Refresh;
                cJSON *add_req = cJSON_CreateObject();
                cJSON *add_list = cJSON_CreateArray();
                cJSON *add_object = cJSON_CreateObject();
                cJSON_AddItemToObject(add_object, "course_id", cJSON_CreateString(courseData->course_id));
                cJSON_AddItemToObject(add_object, "uid", cJSON_CreateString(user_id));
                cJSON_AddItemToArray(add_list, add_object);
                cJSON_AddItemToObject(add_req, "data", add_list);
                ResponseData add_resp = callBackend("course.adminAdd", add_req);
                cJSON_Delete(add_req);
                free(add_resp.raw_string);
                if (add_resp.status_code != 0) {
                    printf("[����ѧ��ʧ��] �����룺%d, %s���������������\n", add_resp.status_code,
                           UTF8ToGBK(findExceptionReason(&add_resp)));
                } else {
                    printf("[����ѧ���ɹ�] �������������\n");
                }
                cJSON_Delete(add_resp.json);
                getch();
                goto Teacher_GetCourseAndDisplay;
            }
            break;
        case 'D':
        case 'd': // ��ѡ�γ�
            if (GLOBAL_user_info.role != 2) break;
            {
                struct teacherCourseSelection *pt = selectedRow->data;
                printf("\n[ȷ����ѡ] ��ȷ��Ҫ��ѡѧ�� %s(%s) �𣿣��������ѧ����ѧ��:%sȷ�ϣ�\n", UTF8ToGBK(pt->student.name),
                       UTF8ToGBK(pt->student.uid), UTF8ToGBK(pt->student.uid));
                char input_char[31];
                gets_safe(input_char, 30);
                if (strcmp(input_char, pt->student.uid) != 0) {
                    printf("ѧ�Ų�һ�£���ȡ�����������������������\n");
                    getch();
                    goto Teacher_GetCourseAndDisplay;
                }
                cJSON *cancel_req_json = cJSON_CreateObject();
                cJSON_AddItemToObject(cancel_req_json, "course_id", cJSON_CreateString(courseData->course_id));
                cJSON_AddItemToObject(cancel_req_json, "uid", cJSON_CreateString(pt->student.uid));
                ResponseData cancel_resp = callBackend("course.adminCancel", cancel_req_json);
                cJSON_Delete(cancel_req_json);
                free(cancel_resp.raw_string);
                if (cancel_resp.status_code != 0) {
                    printf("[��ѡʧ��] �����룺%d, %s���������������\n", cancel_resp.status_code,
                           UTF8ToGBK(findExceptionReason(&cancel_resp)));
                    cJSON_Delete(cancel_resp.json);
                    getch();
                    goto Teacher_GetCourseAndDisplay;
                }
                printf("[��ѡ�ɹ�] ѧ�� %s(%s) �ѱ���ѡ�����������������\n", UTF8ToGBK(pt->student.name), UTF8ToGBK(pt->student.uid));
                getch();
                goto Teacher_GetCourseAndDisplay;
            }
            break;
        case 27:
            goto Teacher_GC_Collect;
        default:
            break;
    }
    goto Teacher_GetKey;

    Teacher_GC_Collect:
    linkListObject_Delete(student_list, 1);
    free(student_list);
    student_list = NULL;
}

/**
 * ���ؼ����޸��Ͽΰ���
 * @param schedule
 * @return
 */
Schedule editSchedule(int schedule[7][13]) {
    HANDLE windowHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (schedule == NULL) {
        schedule = alloca(sizeof(int[7][13]));
        memset(schedule, 0, sizeof(int[7][13]));
    }
    Schedule returnSchedule;
    memcpy(returnSchedule.schedule, schedule, sizeof(int[7][13]));
    int x = 1, y = 0;

    ScheduleEditor_Refresh:
    system("chcp 936>nul & cls & MODE CON COLS=65 LINES=32");
    ui_printHeader_GBK(50);
    printInMiddle_GBK("\n======= �γ̡��༭�ڿΰ��� =======\n", 52);
    printf("-------------------------------------------------------\n");
    printf("   �ڴ�     ��һ  �ܶ�  ����  ����  ����  ����  ����\n");
    printf("-------------------------------------------------------\n");
    for (int i = 1; i <= 12; i++) {
        printf("  ��%2d�ڿ� ", i);
        for (int j = 0; j < 7; j++) {
            if (x == i && y == j) SetConsoleTextAttribute(windowHandle, 0x70);
            printf("  %2s  ", returnSchedule.schedule[j][i] == 1 ? "��" : " ");
            SetConsoleTextAttribute(windowHandle, 0x07);
        }
        if (i == 5 || i == 9) printf("\n"); // ��ʱ��
        printf("\n");
    }

    printf("\n\n<Enter>�л�ѡ��״̬ <Y>�����༭ <ESC>ȡ���༭ <��/��/��/��>�л�\n");

    int key;
    ScheduleEditor_GetKey:
    key = _getch();
    switch (key) {
        case 224:
            key = _getch();
            switch (key) {
                case 80: // ��
                    if (x < 12) x++;
                    else x = 1;
                    goto ScheduleEditor_Refresh;
                case 72: // ��
                    if (x > 1) x--;
                    else x = 12;
                    goto ScheduleEditor_Refresh;
                case 75: // ��
                    if (y > 0) y--;
                    else y = 6;
                    goto ScheduleEditor_Refresh;
                case 77: // ��
                    if (y < 6) y++;
                    else y = 0;
                    goto ScheduleEditor_Refresh;
                default:
                    break;
            }
            break;
        case 13:
            returnSchedule.schedule[y][x] = returnSchedule.schedule[y][x] == 1 ? 0 : 1;
            goto ScheduleEditor_Refresh;
        case 'y':
        case 'Y':
            return returnSchedule;
        case 27:
            memcpy(returnSchedule.schedule, schedule, sizeof(int[7][13]));
            return returnSchedule;
        default:
            break;
    }
    goto ScheduleEditor_GetKey;
}


/**
 * �����γ�
 * @return ȡ���򷵻�NULL�����򷵻ؿγ�ָ��
 */
Course *addCourse() {
    system("chcp 936>nul & cls & MODE CON COLS=75 LINES=55");
    printf("\n");
    ui_printHeader_GBK(69);
    printf("\n");
    printInMiddle_GBK("======= �γ̡�����/�޸Ŀγ� =======\n", 71);

    Course *course = calloc(1, sizeof(Course));
    if (inputStringWithRegexCheck("[�����γ�] ������γ�ID������Ϊ5-30������ĸ�����ֺ�-��ɵ��ַ����������ʽ<ѧ��>-<ѧ��>-<�γ̴���>-<�༶>��\n",
                                  COURSE_ID_PATTERN,
                                  course->course_id,
                                  30) == -1)
        goto CancelAdd;
    if (GLOBAL_user_info.role == 2) { // ����Ա������ָ����ʦ�û�
        if (inputStringWithRegexCheck("[�����γ�] �����뿪�ν�ʦID���û���ɫ��Ϊ��ʦ�����Ա��\n",
                                      USER_PATTERN,
                                      course->teacher.uid,
                                      15) == -1)
            goto CancelAdd;
    } else {
        strcpy(course->teacher.uid, GLOBAL_user_info.userid); // ��ʦ�޷��޸�UID
    }
    if (inputStringWithRegexCheck("[�����γ�] ������γ̱��⣨����Ϊ5-50�ַ�������ĸ�����֡����ĺ�()-��ɵ��ַ�����\n",
                                  COURSE_TITLE_PATTERN,
                                  course->title,
                                  100) == -1)
        goto CancelAdd;
    if (inputStringWithRegexCheck("[�����γ�] ������γ�����������Ϊ5-250�ַ���\n",
                                  COURSE_DESCRIPTION_PATTERN,
                                  course->description,
                                  500) == -1)
        goto CancelAdd;
    if (inputIntWithRegexCheck("[�����γ�] ������γ����ͣ�0-���ޡ�1-ѡ�ޡ�2-��ѡ��3-����\n",
                               COURSE_TYPE_PATTERN,
                               &course->type))
        goto CancelAdd;
    if (inputStringWithRegexCheck("[�����γ�] �����뿪��ѧ��\n",
                                  SEMESTER_PATTERN,
                                  course->semester,
                                  10))
        goto CancelAdd;
    if (inputIntWithRegexCheck("[�����γ�] ������γ̿�ʼ����������0��������\n",
                               NUMBER_PATTERN,
                               &course->week_start))
        goto CancelAdd;
    if (inputIntWithRegexCheck("[�����γ�] ������γ̽�������������0��������\n",
                               NUMBER_PATTERN,
                               &course->week_end))
        goto CancelAdd;
    if (inputFloatWithRegexCheck("[�����γ�] ������γ�ѧ�֣�����0��С������ྫȷ��С�����2λ��\n",
                                 POINTS_PATTERN,
                                 &course->points))
        goto CancelAdd;
    if (inputIntWithRegexCheck("[�����γ�] ���������ѡ������������0��������\n",
                               POINTS_PATTERN,
                               &course->max_members))
        goto CancelAdd;
    printf("[�����γ�] ���������ʼ�༭�ڿΰ���\n");
    getch();
    Schedule ret = editSchedule(NULL);
    memcpy(course->schedule, ret.schedule, sizeof(int[7][13]));

    goto Return;

    CancelAdd:  // ȡ�����
    free(course);
    course = NULL;

    Return:  // ��������
    return course;
}


/**
 * ���ɿα��ŵ�JSON�ַ�
 * @param schedule
 * @return
 */
char *getScheduleJsonString(int schedule[7][13]) {
    char *final_str = (char *) calloc(2000, sizeof(char));
    strcat(final_str, "{");
    if (final_str == NULL) return NULL;
    for (int i = 0; i < 7; i++) {
        char t_str[30];
        sprintf(t_str, "\"%d\":[", i + 1);
        strcat(final_str, t_str);
        char schedule_str[100] = "";
        char tt[10] = "";
        char has_course = 0;
        for (int j = 1; j <= 12; j++) {
            if (schedule[i][j]) {
                if (has_course) strcat(schedule_str, ",");
                has_course = 1;
                sprintf(tt, "%d", j);
                strcat(schedule_str, tt);
            }
        }
        strcat(final_str, schedule_str);
        if (i != 6)
            strcat(final_str, "],");
        else
            strcat(final_str, "]}");
    }
    return final_str;

}


/**
 * ����/�޸Ŀγ�
 *
 * @param _course ��ΪNULLʱ�������γ̣�����Ϊ�޸Ŀγ�
 * @return �γ�ָ��
 */
void editCourse(Course *_course) {
    // ���޿γ���Ϣ���������γ�
    int action = 0;
    Course *course = _course;

    if (course == NULL) {
        course = addCourse();
        action = 1;
        if (course == NULL) {
            return;
        }
    }

    int counter = 0, selected = 0, key;

    EditCourse_Refresh:

    counter = 0;
    system("chcp 936>nul & cls & MODE CON COLS=75 LINES=55");
    printf("\n");
    ui_printHeader_GBK(69);
    printf("\n");
    printInMiddle_GBK("======= �γ̡�����/�޸Ŀγ� =======\n\n", 71);

    selfPlusPrint("\t\t�γ�ID        %s\n", &counter, selected, UTF8ToGBK(course->course_id)); // 0
    selfPlusPrint("\t\t�γ�����      %s\n", &counter, selected, UTF8ToGBK(course->title)); // 1
    selfPlusPrint("\t\t�γ̼��      %s\n", &counter, selected, UTF8ToGBK(course->description)); // 2
    selfPlusPrint("\t\t�γ�����      %s\n", &counter, selected, LECTURE_TYPE[course->type]); // 3
    selfPlusPrint("\t\t����ѧ��      %s\n", &counter, selected, UTF8ToGBK(course->semester)); // 4
    selfPlusPrint("\t\t�ڿ�����      ��%d��~��%d��\n", &counter, selected, course->week_start, course->week_end); // 5
    char *courseArrangeStr = printSchedule(course->schedule); // 6
    if (courseArrangeStr != NULL) {
        selfPlusPrint("\t\t�ڿΰ���      %s\n", &counter, selected, courseArrangeStr);
        free(courseArrangeStr);
    }
    selfPlusPrint("\t\t�ڿν�ʦ      %s(UID:%s)\n", &counter, selected, UTF8ToGBK(course->teacher.name), // 7
                  UTF8ToGBK(course->teacher.uid));
    selfPlusPrint("\t\tѡ������      %d/%d��\n", &counter, selected, course->current_members, course->max_members);  //8
    selfPlusPrint("\t\t�γ�ѧ��      %.2f\n", &counter, selected, course->points);  //9
    selfPlusPrint("\t\t�γ�ѧʱ      %dѧʱ\n", &counter, selected, // 10
                  getTotalWeekHour(course->schedule) * (course->week_end - course->week_start + 1));

    printf("\n");
    printInMiddle_GBK("<Enter>�޸�ѡ���� <Y>�ύ�޸� <Esc>ȡ���޸�", 71);

    EditCourse_GetKey:

    key = _getch();
    switch (key) {
        case 224:
            key = _getch();
            switch (key) {
                case 80: // ��
                    if (selected < 10) selected++;
                    else selected = 0;
                    goto EditCourse_Refresh;
                case 72: // ��
                    if (selected > 0) selected--;
                    else selected = 10;
                    goto EditCourse_Refresh;
                default:
                    break;
            }
            break;
        case 13:
            switch (selected) {
                case 0:
                    inputStringWithRegexCheck("[�޸Ŀγ�] ������γ�ID������Ϊ5-30������ĸ�����ֺ�-��ɵ��ַ����������ʽ<ѧ��>-<ѧ��>-<�γ̴���>-<�༶>��\n",
                                              COURSE_ID_PATTERN,
                                              course->course_id,
                                              30);
                    break;
                case 1:
                    inputStringWithRegexCheck("[�޸Ŀγ�] ������γ̱��⣨����Ϊ5-50�ַ�������ĸ�����֡����ĺ�()-��ɵ��ַ�����\n",
                                              COURSE_TITLE_PATTERN,
                                              course->title,
                                              100);
                    break;
                case 2:
                    inputStringWithRegexCheck("[�޸Ŀγ�] ������γ�����������Ϊ5-250�ַ���\n",
                                              COURSE_DESCRIPTION_PATTERN,
                                              course->description,
                                              500);
                    break;
                case 3:
                    inputIntWithRegexCheck("[�޸Ŀγ�] ������γ����ͣ�0-���ޡ�1-ѡ�ޡ�2-��ѡ��3-����\n",
                                           COURSE_TYPE_PATTERN,
                                           &course->type);
                    break;
                case 4:
                    inputStringWithRegexCheck("[�޸Ŀγ�] �����뿪��ѧ��\n",
                                              SEMESTER_PATTERN,
                                              course->semester,
                                              10);
                    break;
                case 5:
                    if (inputIntWithRegexCheck("[�޸Ŀγ�] ������γ̿�ʼ����������0��������\n",
                                               NUMBER_PATTERN,
                                               &course->week_start) == -1)
                        break;
                    inputIntWithRegexCheck("[�޸Ŀγ�] ������γ̽�������������0��������\n",
                                           NUMBER_PATTERN,
                                           &course->week_end);
                    break;
                case 6: {
                    Schedule ret = editSchedule(course->schedule);
                    memcpy(course->schedule, ret.schedule, sizeof(int[7][13]));
                }
                    break;
                case 7:
                    // ����Ա���޸Ŀ��ν�ʦ
                    if (GLOBAL_user_info.role == 2) {
                        inputStringWithRegexCheck("[�޸Ŀγ�] �����뿪�ν�ʦID���û���ɫ��Ϊ��ʦ�����Ա��\n",
                                                  USER_PATTERN,
                                                  course->teacher.uid,
                                                  15);
                    }
                    break;
                case 9:
                    inputFloatWithRegexCheck("[�޸Ŀγ�] ������γ�ѧ�֣�����0��С������ྫȷ��С�����2λ��\n",
                                             POINTS_PATTERN,
                                             &course->points);
                    break;
                case 8:
                    inputIntWithRegexCheck("[�޸Ŀγ�] ���������ѡ������������0��������\n",
                                           POINTS_PATTERN,
                                           &course->max_members);
                    break;
                default:
                    break;
            }
            goto EditCourse_Refresh;
        case 'y':
        case 'Y':
            if (course->week_start <= course->week_end && course->week_start > 0 && course->max_members > 0 &&
                course->points > 0) {
                printf("\n[��ʾ] �������������...\n");
                cJSON *req_json = cJSON_CreateObject();
                cJSON_AddItemToObject(req_json, "action", cJSON_CreateNumber(action));
                cJSON_AddItemToObject(req_json, "course_id", cJSON_CreateString(course->course_id));
                cJSON_AddItemToObject(req_json, "title", cJSON_CreateString(course->title));
                cJSON_AddItemToObject(req_json, "teacher", cJSON_CreateString(course->teacher.uid));
                cJSON_AddItemToObject(req_json, "semester", cJSON_CreateString(course->semester));
                cJSON_AddItemToObject(req_json, "description", cJSON_CreateString(course->description));
                cJSON_AddItemToObject(req_json, "max_members", cJSON_CreateNumber(course->max_members));
                cJSON_AddItemToObject(req_json, "points", cJSON_CreateNumber(course->points));
                cJSON_AddItemToObject(req_json, "week_start", cJSON_CreateNumber(course->week_start));
                cJSON_AddItemToObject(req_json, "week_end", cJSON_CreateNumber(course->week_end));
                cJSON_AddItemToObject(req_json, "type", cJSON_CreateNumber(course->type));
                cJSON_AddItemToObject(req_json, "schedule",
                                      cJSON_CreateString(getScheduleJsonString(course->schedule)));
                ResponseData resp = callBackend("course.submit", req_json);
                free(req_json);
                free(resp.raw_string);
                if (resp.status_code != 0) {
                    printf("[�ύʧ��] �����룺%d, %s(�����������)\n", resp.status_code, UTF8ToGBK(findExceptionReason(&resp)));
                    getch();
                    cJSON_Delete(resp.json);
                    goto EditCourse_Refresh;
                }
                cJSON_Delete(resp.json);
                printf("[�ύ�ɹ�] �γ��޸�/�����ɹ�������������ؿγ�һ����\n");
                getch();
                goto GC_COLLECT;
            } else {
                printf("[�ύʧ��] ������ӦС�ڵ��ڽ����ܣ��ҿ����ܡ�ѧ�ֺ��������Ӧ����0��(�����������)\n");
                getch();
                goto EditCourse_Refresh;
            }
            break;
        case 27:
            goto GC_COLLECT;
        default:
            break;
    }
    goto EditCourse_GetKey;

    GC_COLLECT:
    if (_course == NULL) free(course);
}


/**
 * ����ѧ��ѡ���б�
 */
void importStuCourseData() {

    typedef struct _import_data {
        char uid[21], course_id[33];
    } ImportData;

    LinkList_Object *import_list = linkListObject_Init(); // ��ʼ������
    int page = 1, max_page, page_size = 30, total = 0, key;

    Import_Refresh:

    max_page = (int) ceilf((float) total / (float) page_size);
    system("chcp 936>nul & cls & MODE CON COLS=75 LINES=55");
    printf("\n");
    ui_printHeader_GBK(54);
    printf("\n");
    printInMiddle_GBK("======= �γ̡�����ѧ������ =======\n\n", 56);
    printf("%-6s%-20s%-30s\n", "���", "ѧ��", "ѡ�޿γ�ID");
    printf("--------------------------------------------------------\n");
    int counter = 0, current_total = 0;
    for (LinkList_Node *pt = import_list->head; pt != NULL; pt = pt->next) {
        counter++;
        if (ceilf((float) counter / (float) page_size) < (float) page) continue; // ��λ����ҳ�棬ѧ��������ҳ��ʾ
        if (ceilf((float) counter / (float) page_size) > (float) page) break; // һҳ��ʾ�꼴������ѭ��
        current_total++;
        ImportData *tmp = pt->data;
        printf("%4d  %-20s%-30s\n",
               counter,
               UTF8ToGBK(tmp->uid),
               UTF8ToGBK(tmp->course_id));
        if (pt->next == NULL) { // ��������һ���ڵ�
            printf("\n");
        }
    }
    for (int i = 0; i < page_size - current_total - 1; i++) printf("\n"); // ����ҳ��
    printf("\n");
    printInMiddle_GBK("=============================\n", 56);
    printf("    [��ʾ] ��%4d�����ݣ���ǰ��%3dҳ����%3dҳ\n\n    ���������ǰһҳ���ҷ��������һҳ��\n\n",
           total, page, max_page);
    printInMiddle_GBK("<T>����ģ�� <I>ѡ���ļ� <Y>ȷ�ϵ��� <Esc>ȡ��������", 56);
    printf("\n");

    Import_GetKey:

    key = _getch();
    switch (key) {
        case 224:
            key = _getch();
            switch (key) {
                case 75: // ��
                    if (page > 1) page--;
                    else break;
                    goto Import_Refresh;
                case 77: // ��
                    if (page < max_page) page++;
                    else break;
                    goto Import_Refresh;
                default:
                    break;
            }
            break;
        case 'T':
        case 't': {
            FILE *fp = fopen("import_template.csv", "w");
            if (fp == NULL) {
                printf("[��ʾ] ����ģ��ʧ�ܣ������ļ�Ȩ�ޡ����������������\n");
                getch();
                goto Import_Refresh;
            }
            fprintf(fp, "ѧ��,�γ�ID\n2135060620(ʾ������ɾ��),2022-01-0000001-01(ʾ������ɾ��)\n");
            fclose(fp);
            printf("[��ʾ] ����ģ��ɹ���ģ�屣����\"import_template.csv\"�ļ��С����������������\n");
            goto Import_Refresh;
        }
        case 'Y':
        case 'y': { // ��������
            if (total == 0) {
                printf("[����ѧ��ʧ��] �б�Ϊ�գ����Ȱ�\"I\"�������ݣ��������������\n");
                getch();
                goto Import_Refresh;
            }
            cJSON *add_req = cJSON_CreateObject();
            cJSON *add_list = cJSON_CreateArray();
            for (LinkList_Node *pt = import_list->head; pt != NULL; pt = pt->next) {
                cJSON *add_object = cJSON_CreateObject();
                cJSON_AddItemToObject(add_object, "course_id",
                                      cJSON_CreateString(((ImportData *) pt->data)->course_id));
                cJSON_AddItemToObject(add_object, "uid", cJSON_CreateString(((ImportData *) pt->data)->uid));
                cJSON_AddItemToArray(add_list, add_object);
            }
            cJSON_AddItemToObject(add_req, "data", add_list);
            ResponseData add_resp = callBackend("course.adminAdd", add_req);
            cJSON_Delete(add_req);
            free(add_resp.raw_string);
            if (add_resp.status_code != 0) {
                printf("[����ѧ��ʧ��] �����룺%d, %s���������������\n", add_resp.status_code,
                       UTF8ToGBK(findExceptionReason(&add_resp)));
                getch();
                goto Import_Refresh;
            } else {
                printf("[����ѧ���ɹ�] �������������\n");
                cJSON_Delete(add_resp.json);
                getch();
                goto Import_GC_Collect;
            }
            break;
        }
        case 'I': // ѡ���ļ�����
        case 'i': {
            printf("[��ʾ] ���ڴ򿪵ĶԻ�����ѡ���ļ�������ɽ�һ���Ĳ���...\n");
            char file_path[260] = {0};
            if (openFileDialog(file_path, "TemplateFile\0*.csv\0", "��ѡ����ģ��") == 0) {
                printf("[��ʾ] �û�ȡ�����ļ�ѡ�񣨰������������\n");
                getch();
                goto Import_Refresh;
            }
            char buf[1024];
            FILE *fp = fopen(file_path, "r");
            if (fp == NULL) {
                printf("[��ʾ] ���ļ�ʧ�ܣ��������������\n");
                getch();
                goto Import_Refresh;
            }
            // ������ϣ������ȥ���ظ�����
            SimpleHashList *hash_list = hashList_init();
            fgets(buf, sizeof(buf), fp);  // ������һ��
            while (fgets(buf, sizeof(buf), fp)) {
                if (hashList_findString(hash_list, buf)) continue; // ����Ѿ����ڸ����ݣ���ֱ����һ��
                hashList_appendString(hash_list, buf);// �����ϣ��
                ImportData *imp = calloc(1, sizeof(ImportData)); // �����ڴ�
                buf[strlen(buf) - 1] = '\0'; // ȥ����β����
                char *p;
                char *ptr = strtok_r(buf, ",", &p); // �ַ����ָ�
                if (ptr == NULL) break; // ������һ�����л���Ч��
                strcpy(imp->uid, GBKToUTF8(ptr));
                ptr = strtok_r(NULL, ",", &p); // �ַ����ָ�
                strcpy(imp->course_id, GBKToUTF8(ptr));
                if (regexMatch(USER_PATTERN, imp->uid) && regexMatch(COURSE_ID_PATTERN, imp->course_id)) {
                    linkListObject_Append(import_list, imp);
                    total++;
                } else {
                    free(imp);
                }
            }
            fclose(fp);
            hashList_delList(hash_list);// �ͷŹ�ϣ��ռ���ڴ�
            printf("[����ɹ�] ��ϣ��ȥ�ز�У��󣬹�����%d����Ч���ݣ��������������\n", total);
            getch();
            goto Import_Refresh;
            break;
        }
        case 27:
            goto Import_GC_Collect;
        default:
            break;
    }
    goto Import_GetKey;

    Import_GC_Collect:
    linkListObject_Delete(import_list, 1);
    free(import_list);
}