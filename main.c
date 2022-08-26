extern __attribute__((stdcall)) void f_stdcall(int, int);

extern __attribute__((cdecl)) void f_cdecl(int, int);

__attribute__((noinline, stdcall))
static void main_stdcall() {
   f_stdcall(123, 456);
}

__attribute__((noinline, cdecl))
static void main_cdecl() {
   f_cdecl(123, 456);
}

static void busyloop() {
   for (;;);
}

__attribute__((noinline, stdcall))
static void main_stdcall_fun() {
   f_stdcall(123, (int) busyloop);
}

int main(int c, char *v[]) {
   if (!v[1])
      return 1;

   switch (v[1][0]) {
      case 'c':
         main_cdecl();
         break;
      case 's':
         main_stdcall();
         break;
      case 'f':
         main_stdcall_fun();
         break;
   }
}
