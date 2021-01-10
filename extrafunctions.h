// kbhit for linux gcc

extern int KBinitialized;

#ifdef __cplusplus
extern "C" {
#endif
int kbhit();
void kbinit();
void kbfini();
double deg2rad(double);
double rad2deg(double);
int substr(char str[],char substr[]);
#ifdef __cplusplus
}
#endif
