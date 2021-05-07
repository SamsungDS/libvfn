ccan_config = configuration_data()

ccan_config.set10('HAVE_ATTRIBUTE_COLD',
  cc.has_function_attribute('cold'),
)

ccan_config.set10('HAVE_ATTRIBUTE_CONST',
  cc.has_function_attribute('const'),
)

ccan_config.set10('HAVE_ATTRIBUTE_DEPRECATED',
  cc.has_function_attribute('deprecated'),
)

ccan_config.set10('HAVE_ATTRIBUTE_NONNULL',
  cc.has_function_attribute('nonnull'),
)

ccan_config.set10('HAVE_ATTRIBUTE_NORETURN',
  cc.has_function_attribute('noreturn'),
)

ccan_config.set10('HAVE_ATTRIBUTE_PRINTF',
  cc.has_function_attribute('format'),
)

ccan_config.set10('HAVE_ATTRIBUTE_PURE',
  cc.has_function_attribute('pure'),
)

ccan_config.set10('HAVE_ATTRIBUTE_RETURNS_NONNULL',
  cc.has_function_attribute('returns_nonnull'),
)

ccan_config.set10('HAVE_ATTRIBUTE_SENTINEL',
  cc.compiles('''
    #include <stddef.h>

    static __attribute__((sentinel)) void func(int x, ...) { }

    int main(void) {
      (void)func;
      return 0;
    }
  ''', name: 'have __attribute__((sentinel))',
  args: ['-Werror']),
)

ccan_config.set10('HAVE_ATTRIBUTE_UNUSED',
  cc.has_function_attribute('unused'),
)

ccan_config.set10('HAVE_ATTRIBUTE_USED',
  cc.has_function_attribute('used'),
)

ccan_config.set10('HAVE_BUILTIN_CHOOSE_EXPR',
  cc.has_function('__builtin_choose_expr'),
)

ccan_config.set10('HAVE_BUILTIN_CONSTANT_P',
  cc.has_function('__builtin_constant_p'),
)

ccan_config.set10('HAVE_BUILTIN_CPU_SUPPORTS',
  cc.has_function('__builtin_cpu_supports'),
)

ccan_config.set10('HAVE_BUILTIN_EXPECT',
  cc.has_function('__builtin_expect'),
)

ccan_config.set10('HAVE_BUILTIN_TYPES_COMPATIBLE_P',
  cc.has_function('__builtin_types_compatible_p')
)

ccan_config.set10('HAVE_STRUCT_TIMESPEC',
  cc.has_type('struct timespec',
    prefix: '#include <time.h>',
  ),
)

ccan_config.set10('HAVE_CLOCK_GETTIME',
  cc.has_function('clock_gettime',
    prefix: '#include <time.h>',
  ),
)

ccan_config.set10('HAVE_COMPOUND_LITERALS',
  cc.compiles('''
    int main(void) {
      int *foo = (int[]){1, 2, 3, 4};
      return 0;
    }
  ''', name: 'have compound literals',
  args: ['-Werror']),
)

ccan_config.set10('HAVE_ERR_H',
  cc.check_header('err.h')
)

ccan_config.set10('HAVE_ISBLANK',
  cc.has_function('isblank',
    args: ['-D_GNU_SOURCE'],
    prefix: '#include <ctype.h>',
  ),
)

ccan_config.set10('HAVE_STATEMENT_EXPR',
  cc.compiles('''
    int main(void) {
      return ({ int x = 1; x; });
    }
  ''', name: 'have statement expressions',
  args: ['-Werror']),
)

ccan_config.set10('HAVE_SYS_TERMIOS_H',
  cc.check_header('sys/termios.h'),
)

ccan_config.set10('HAVE_SYS_UNISTD_H',
  cc.check_header('sys/unistd.h'),
)

ccan_config.set10('HAVE_TYPEOF',
  cc.compiles('''
    int main(void) {
      int x;
      __typeof__(x) y;
      y = x;
      return y == x ? 0 : 1;
    }
  ''', name: 'have __typeof__',
  args: ['-Werror'])
)

ccan_config.set10('HAVE_WARN_UNUSED_RESULT',
  cc.has_function_attribute('warn_unused_result'),
)

ccan_config_h = configure_file(
  output: 'config.h',
  configuration: ccan_config,
)

ccan_inc = include_directories('.')

ccan_sources = [
  'ccan/err/err.c',
  'ccan/list/list.c',
  'ccan/opt/helpers.c',
  'ccan/opt/opt.c',
  'ccan/opt/parse.c',
  'ccan/opt/usage.c',
  'ccan/str/str.c',
  'ccan/tap/tap.c',
  'ccan/time/time.c',
]

ccan_lib = static_library('ccan',
  [ccan_config_h, ccan_sources],
  include_directories : ccan_inc,
)
