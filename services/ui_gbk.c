//
// Created by admin on 2022/7/2.
//

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <stdarg.h>
#include <string.h>
#include <Windows.h>
#include "pcre2.h"
#include "ui_gbk.h"
#include "global.h"
#include "string_ext.h"

#define TITLE() system("title ѧ��ѡ��ϵͳ - " VERSION)

/**
 * ���ó�����⣨GBK��
 */
void setTitle() {
    TITLE();
}

/**
 * ���������Ϣ
 * @param msg
 */
void printErrorData(char *msg) {
    printf("[ϵͳ����] %s���������������\n", msg);
    getch();
}


/**
 * ���������Ϣ
 * @param msg ��Ϣ
 * @param width ���
 */
void printInMiddle_GBK(char *msg, int width, ...) {
    unsigned int str_length = (unsigned int) strlen(msg);
    char *final_str = calloc(str_length + 1000, sizeof(char));
    // �Ƚ���ʽ���Ľ��������ַ�����
    va_list args;
    va_start(args, width);
    vsnprintf(final_str, str_length + 1000, msg, args);
    va_end(args);
    // ����������Ҫռ�õĳ���
    str_length = strlen(final_str);
    if (width - (int) str_length > 0) {
        for (int i = 0; (i < (width - str_length) / 2); i++) printf(" ");
    }
    printf("%s\n", final_str);
}


/**
 * ������ʽ�������ݲ���֤
 * @param message ��ʾ��Ϣ
 * @param pattern ������ʽ
 * @param _dest Ŀ���ַ���
 * @param max_length �����볤��
 *
 * @return 0 ���� -1 �û�ȡ��
 */
int inputStringWithRegexCheck(char *message, char *pattern, char *_dest, int max_length) {
    char *input_string;
    printf(message);

    InputString:
    printf("[����������(Escȡ��)] ");
    input_string = calloc(max_length + 1, sizeof(char));
    int str_len = 0, t; //��¼����
    while (1) {
        t = _getch();
        if (t == 27) {
            return -1; // �û��˳�
        }
        if (t == '\r' || t == '\n') { //�����س��������������
            printf("\n");
            break; //while ѭ���ĳ���
        } else if (t == '\b') { //�����˸���Ҫɾ��ǰһ���Ǻ�
            if (str_len <= 0) continue; // ������Ȳ��㣬��ִ���˸����
            printf("\b \b");  //�˸񣬴�һ���ո����˸�ʵ�������ÿո񸲸ǵ��Ǻ�
            --str_len;
        } else {
            if (str_len >= max_length) continue; // �������볤��
            input_string[str_len++] = (char) t;//���ַ���������
            printf("%c", t);
        }
    }

    char *final_str = GBKToUTF8(input_string); // ��ת����UTF8��ʽ
    if (regexMatch(pattern, final_str) == 0) {
        free(input_string);
        printf("��������ݲ����Ϲ�������������\n");
        goto InputString;
    }

    strcpy(_dest, final_str);
    free(input_string);
    return 0;
}


/**
 * ������ʽ�������ݲ���֤
 * @param message ��ʾ��Ϣ
 * @param pattern ������ʽ
 * @param _dest Ŀ���ַ���
 * @param max_length �����볤��
 *
 * @return 0 ���� -1 �û�ȡ��
 */
int inputIntWithRegexCheck(char *message, char *pattern, int *_dest) {
    char *input_string;
    InputInteger:
    input_string = calloc(12, sizeof(char));
    if (inputStringWithRegexCheck(message, pattern, input_string, 11) == -1) {
        return -1;
    }
    char *end_ptr;
    int result = strtol(input_string, &end_ptr, 10);
    if (*end_ptr != 0) {
        printf("��������ȷ������\n");
        free(input_string);
        goto InputInteger;
    }
    *_dest = result;
    return 0;
}

/**
 * ������ʽ�������ݲ���֤��С����
 * @param message ��ʾ��Ϣ
 * @param pattern ������ʽ
 * @param _dest Ŀ���ַ���
 * @param max_length �����볤��
 *
 * @return 0 ���� -1 �û�ȡ��
 */
int inputFloatWithRegexCheck(char *message, char *pattern, double *_dest) {
    char *input_string;
    InputFloat:
    input_string = calloc(12, sizeof(char));
    if (inputStringWithRegexCheck(message, pattern, input_string, 11) == -1) {
        return -1;
    }
    char *end_ptr;
    double result = strtof(input_string, &end_ptr);
    if (*end_ptr != 0) {
        printf("��������ȷ������\n");
        free(input_string);
        goto InputFloat;
    }
    *_dest = result;
    return 0;
}

/**
 * �����������
 *
 * @param format
 * @param counter
 * @param selected ��Ҫ��������
 * @param args
 */
void selfPlusPrint(char *format, int *counter, int selected, ...) {
    HANDLE windowHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (*counter == selected) {
        SetConsoleTextAttribute(windowHandle, 0x70);
    }

    va_list args;
    va_start(args, selected);
    // ���̶�����ͨ��va_list���棬ʹ��vprintf������ɸ��õط�װprint����
    vprintf(format, args);
    va_end(args);

    SetConsoleTextAttribute(windowHandle, 0x07);
    (*counter)++;
}


/**
 * UIģ�飬��ӡ�������ơ��汾�ŵ���Ϣ
 */
void ui_printHeader_GBK(int width) {
    printInMiddle_GBK("----------------------------", width);
    printInMiddle_GBK("|        ѧ��ѡ��ϵͳ        |", width);
    printInMiddle_GBK("|   %s   |", width, VERSION);
    printInMiddle_GBK("----------------------------", width);
}

