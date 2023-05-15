#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ticker.h"

// int matches_watcher_table(char* input) {
//   regex_t regex;
//   int ret;

//   // Compile the regex
//   // ret =
//   //     regcomp(&regex,
//   //             "^(ticker> )*[0-9]+\t[a-zA-Z.]+\\([+-]?[0-9]+,[0-9],[0-9]\\)(\n| "
//   //             "([a-zA-Z. ]+)* (\\[[a-zA-Z. ]+\\]\n))(ticker> )*$",
//   //             REG_EXTENDED);
//   ret = regcomp(&regex,"^(ticker> )* [a-zA-Z. 0-9]+", REG_EXTENDED);
//   if (ret) {
//     // fprintf(stderr, "Could not compile regex\n");
//     regfree(&regex);
//     return 0;
//   }

//   int end = 0;

//   // Match input against regex
//   ret = regexec(&regex, input, 0, NULL, 0);
//   if (!ret) {
//     // printf("Input matches regex\n");
//     end = 1;
//   } else if (ret == REG_NOMATCH) {
//     printf("Input does not match regex\n");
//     size_t error_len = regerror(ret, &regex, NULL, 0);
//     char* error = malloc(error_len);
//     regerror(ret, &regex, error, error_len);

//     printf("Failed to match: %s\n", error);

//     // regerror(ret, &regex, input, sizeof(input));

//     end = 0;
//   } else {
//     regerror(ret, &regex, input, sizeof(input));
//   }

//   regfree(&regex);
//   return end;
// }

// char* remove_all_ticker_text(char *filename) {
//     FILE* fp = fopen(filename, "r");
//     if (fp == NULL) {
//         perror("Error opening file");
//         return NULL;
//     }

//     char* line = NULL;
//     size_t len = 0;
//     ssize_t read;
//     char* output = malloc(1);
//     output[0] = '\0';
//     while ((read = getline(&line, &len, fp)) != -1) {
//         int tick_cnt = 0;
//         while (strncmp(line, "ticker>", 7) == 0){
//           tick_cnt += 7;
//         }
//         output = realloc(output, strlen(output) + (strlen(line)-tick_cnt) +
//         1); strcat(output, line+tick_cnt);
//     }
//     free(line);
//     fclose(fp);
//     return output;
// }

// char* read_text_from_file(const char* filename) {
//   FILE* fp = fopen(filename, "r");
//   if (fp == NULL) {
//     printf("Error: Could not open file '%s'\n", filename);
//     return NULL;
//   }

//   // Determine the size of the file
//   fseek(fp, 0L, SEEK_END);
//   long int file_size = ftell(fp);
//   rewind(fp);

//   // Allocate memory to store the file contents
//   char* file_contents = malloc(sizeof(char) * (file_size + 1));
//   if (file_contents == NULL) {
//     printf("Error: Could not allocate memory\n");
//     fclose(fp);
//     return NULL;
//   }

//   // Read the file contents into memory
//   size_t num_read = fread(file_contents, sizeof(char), file_size, fp);
//   if (num_read != file_size) {
//     printf("Error: Could not read entire file\n");
//     fclose(fp);
//     free(file_contents);
//     return NULL;
//   }

//   // Add a null terminator to the end of the string
//   file_contents[file_size] = '\0';

//   fclose(fp);
//   return file_contents;
// }

// Test(ticker_suite, watchers_format) {
//   char* cmd =
//       "(echo start bitstamp.net live_trades_btcusd; echo watchers; echo quit) "
//       "| timeout -s KILL 5s bin/ticker > test_output/watchers_format.out";
//   int return_code = WEXITSTATUS(system(cmd));
//   cr_assert(return_code == EXIT_SUCCESS,
//             "Program exited with %d instead of EXIT_SUCCESS", return_code);
//   char* watchers_txt = read_text_from_file("test_output/watchers_formattest.out");

//   char* expected_txt = "ticker> ticker> 0\tCLI(-1,0,1)";
//   // hold text 
//   char* hold_txt = watchers_txt;
//   while(strncmp(watchers_txt, "ticker>", 7) == 0){
//     watchers_txt += 7;
//   }
//   if (strncmp(watchers_txt, "0\tCLI(-1,0,1)\n", strlen("0\tCLI(-1,0,1)\n")) != 0) {
//     cr_assert_fail("Expected: %s\nActual: %s\n", expected_txt, watchers_txt);
//   }
//   watchers_txt+=strlen("0\tCLI(-1,0,1)\n");
//   // match regex to (number,number,number)
//   regex_t regex;
//   int ret;
//   ret = regcomp(&regex, "^[0-9]+\t[a-zA-Z.]+\\([+-]?[0-9]+,[0-9],[0-9]\\)", REG_EXTENDED);




//   free(watchers_txt);
// }
// idc im not running a pipe
// Test(ticker_suite, survive_kill_uwsc) {
//   char* cmd =
//       "(echo start bitstamp.net live_trades_btcusd; echo watchers; echo quit) "
//       "| timeout -s KILL 5s bin/ticker > test_output/watchers_format.out";
//   int return_code = WEXITSTATUS(system(cmd));
//   system("pkill -9 uwsc");
// }

Test(basecode_suite, stop_1_not_crash_test) {
    char *cmd = "(echo start bitstamp.net live_trades_btcusd; echo stop 1; echo quit) | timeout -s KILL 5s bin/ticker > test_output/stop_1_not_crash.out";
    char *cmp = "cmp test_output/stop_1_not_crash.out tests/rsrc/stop_1_not_crash.out";
    int return_code = WEXITSTATUS(system(cmd));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with %d instead of EXIT_SUCCESS",
		 return_code);

    return_code = WEXITSTATUS(system(cmp));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program output did not match reference output.");
}

Test(basecode_suite, question_mark) {
    char *cmd = "(echo what is this; echo quit) | timeout -s KILL 5s bin/ticker > test_output/question_mark.out";
    char *cmp = "cmp test_output/question_mark.out tests/rsrc/question_mark.out";

    int return_code = WEXITSTATUS(system(cmd));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with %d instead of EXIT_SUCCESS",
		 return_code);

    return_code = WEXITSTATUS(system(cmp));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program output did not match reference output.");
}

Test(basecode_suite, no_arg_start) {
    char *cmd = "echo start bitstamp.net | timeout -s KILL 5s bin/ticker > test_output/no_arg_start.out";
    char *cmp = "cmp test_output/no_arg_start.out tests/rsrc/no_arg_start.out";

    int return_code = WEXITSTATUS(system(cmd));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with %d instead of EXIT_SUCCESS",
		 return_code);

    return_code = WEXITSTATUS(system(cmp));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program output did not match reference output.");
}

// Test(basecode_suite, quit_test) {
//     char *cmd = "(echo start bitstamp.net) | timeout -s KILL 5s bin/ticker > test_output/quit.out";
//     char *cmp = "cmp test_output/no_arg_start.out tests/rsrc/quit.out";

//     int return_code = WEXITSTATUS(system(cmd));
//     cr_assert_eq(return_code, EXIT_SUCCESS,
//                  "Program exited with %d instead of EXIT_SUCCESS",
// 		 return_code);

//     return_code = WEXITSTATUS(system(cmp));
//     cr_assert_eq(return_code, EXIT_SUCCESS,
//                  "Program output did not match reference output.");
// }


