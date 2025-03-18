//===--- taxi_mt_unsafe_check.cpp - clang-tidy ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "taxi_mt_unsafe_check.h"
#include <contrib/libs/clang14/include/clang/AST/ASTContext.h>
#include <contrib/libs/clang14/include/clang/ASTMatchers/ASTMatchFinder.h>

using namespace clang::ast_matchers;
using namespace clang::ast_matchers::internal;

namespace clang {
namespace tidy {
namespace misc {

namespace {
Matcher<FunctionDecl> hasAnyMtUnsafeNames() {
  static const std::vector<StringRef> functions = {
      "::argp_error",
      "::argp_help",
      "::argp_parse",
      "::argp_state_help",
      "::argp_usage",
      "::asctime",
      "::clearenv",
      "::crypt",
      "::ctime",
      "::cuserid",
      "::drand48",
      "::ecvt",
      "::encrypt",
      "::endfsent",
      "::endgrent",
      "::endhostent",
      "::endnetent",
      "::endnetgrent",
      "::endprotoent",
      "::endpwent",
      "::endservent",
      "::endutent",
      "::endutxent",
      "::erand48",
      "::error_at_line",
      "::exit",
      "::fcloseall",
      "::fcvt",
      "::fgetgrent",
      "::fgetpwent",
      "::gammal",
      "::getchar_unlocked",
      "::getdate",
      "::getfsent",
      "::getfsfile",
      "::getfsspec",
      "::getgrent",
      "::getgrent_r",
      "::getgrgid",
      "::getgrnam",
      "::gethostbyaddr",
      "::gethostbyname",
      "::gethostbyname2",
      "::gethostent",
      "::getlogin",
      "::getmntent",
      "::getnetbyaddr",
      "::getnetbyname",
      "::getnetent",
      "::getnetgrent",
      "::getnetgrent_r",
      "::getopt",
      "::getopt_long",
      "::getopt_long_only",
      "::getpass",
      "::getprotobyname",
      "::getprotobynumber",
      "::getprotoent",
      "::getpwent",
      "::getpwent_r",
      "::getpwnam",
      "::getpwuid",
      "::getservbyname",
      "::getservbyport",
      "::getservent",
      "::getutent",
      "::getutent_r",
      "::getutid",
      "::getutid_r",
      "::getutline",
      "::getutline_r",
      "::getutxent",
      "::getutxid",
      "::getutxline",
      "::getwchar_unlocked",
      "::glob",
      "::glob64",
      "::gmtime",
      "::hcreate",
      "::hdestroy",
      "::hsearch",
      "::innetgr",
      "::jrand48",
      "::l64a",
      "::lcong48",
      "::lgammafNx",
      "::localeconv",
      "::localtime",
      "::login",
      "::login_tty",
      "::logout",
      "::logwtmp",
      "::lrand48",
      "::mallinfo",
      "::mallopt",
      "::mblen",
      "::mbrlen",
      "::mbrtowc",
      "::mbsnrtowcs",
      "::mbsrtowcs",
      "::mbtowc",
      "::mcheck",
      "::mprobe",
      "::mrand48",
      "::mtrace",
      "::muntrace",
      "::nice",
      "::nrand48",
      "::pause",
      "::__ppc_get_timebase_freq",
      "::ptsname",
      "::putchar_unlocked",
      "::putenv",
      "::pututline",
      "::pututxline",
      "::putwchar_unlocked",
      "::qecvt",
      "::qfcvt",
      "::register_printf_function",
      "::seed48",
      "::setenv",
      "::setfsent",
      "::setgrent",
      "::sethostent",
      "::sethostid",
      "::setkey",
      "::setlocale",
      "::setlogmask",
      "::setnetent",
      "::setnetgrent",
      "::setprotoent",
      "::setpwent",
      "::setservent",
      "::setutent",
      "::setutxent",
      "::siginterrupt",
      "::sigpause",
      "::sigprocmask",
      "::sigsuspend",
      "::sleep",
      "::srand48",
      "::strerror",
      "::strsignal",
      "::strtok",
      "::tcflow",
      "::tcsendbreak",
      "::tmpnam",
      "::ttyname",
      "::unsetenv",
      "::updwtmp",
      "::utmpname",
      "::utmpxname",
      "::valloc",
      "::vlimit",
      "::wcrtomb",
      "::wcsnrtombs",
      "::wcsrtombs",
      "::wctomb",
      "::wordexp",
  };

  return hasAnyName(functions);
}

} // namespace

TaxiMtUnsafeCheck::TaxiMtUnsafeCheck(StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {}

void TaxiMtUnsafeCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      callExpr(callee(functionDecl(hasAnyMtUnsafeNames()))).bind("mt-unsafe"),
      this);
}

void TaxiMtUnsafeCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Call = Result.Nodes.getNodeAs<CallExpr>("mt-unsafe");
  assert(Call && "Unhandled binding in the Matcher");

#if 0
  const auto *Callee = Call->Callee;
  assert(Callee && "Unhandled binding in the Matcher (2)");

  const auto *CalleeDecl = Callee->getCalleeDecl();
  assert(CalleeDecl && "Unhandled binding in the Matcher (3)");
#endif

  diag(Call->getBeginLoc(), "function is not thread safe")
      << SourceRange(Call->getBeginLoc(), Call->getEndLoc());
}

} // namespace misc
} // namespace tidy
} // namespace clang
