// encoding: UTF-8
/**
 * @file  OnigurumaRegExEngine.cxx
 * @brief integrate Oniguruma regex engine for Scintilla library
 *        (Scintilla Lib is copyright 1998-2017 by Neil Hodgson <neilh@scintilla.org>)
 *
 *        uses Oniguruma - Regular Expression Engine (v6.9.0) (oniguruma.h) - https://github.com/kkos/oniguruma
 *
 *   See also the Wiki page: https://github.com/kkos/oniguruma/wiki
 *
 *
 * @autor Rainer Kottenhoff (RaiKoHoff)
 *
 * TODO: add interface to onig_scan() API (mark occ, hyperlink)
 */

#ifdef SCI_OWNREGEX

#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <mbstring.h>

#define VC_EXTRALEAN 1
#include <windows.h>

#include "Platform.h"
#include "Scintilla.h"
#include "ILexer.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "CellBuffer.h"
#include "CaseFolder.h"
#include "RunStyles.h"
#include "Decoration.h"
#include "CharClassify.h"
#include "Document.h"

// ---------------------------------------------------------------
#ifdef ONIG_ESCAPE_UCHAR_COLLISION
#undef ONIG_ESCAPE_UCHAR_COLLISION
#endif
#include "../oniguruma/src/oniguruma.h"   // Oniguruma - Regular Expression Engine (v6.9.2)
// ---------------------------------------------------------------

#define UCharPtr(pchar) reinterpret_cast<OnigUChar*>(pchar)
#define UCharCPtr(pchar) reinterpret_cast<const OnigUChar*>(pchar)

using namespace Scintilla;

#define SciPos(pos)     static_cast<Sci::Position>(pos)
#define SciLn(line)     static_cast<Sci::Line>(line)
#define SciPosExt(pos)  static_cast<Sci_Position>(pos)

// ============================================================================
// ***   Oningmo configuration   ***
// ============================================================================

enum class EOLmode : int { CRLF = SC_EOL_CRLF, CR = SC_EOL_CR, LF = SC_EOL_LF };

//static OnigEncoding s_UsedEncodingsTypes[] = { ONIG_ENCODING_UTF8 };
static OnigEncoding s_UsedEncodingsTypes[] = { ONIG_ENCODING_UTF8, ONIG_ENCODING_UTF8_CR, ONIG_ENCODING_UTF8_CRLF };

// ============================================================================
// ============================================================================

// ------------------------------------
// --- Onigmo Engine Simple Options ---
// ------------------------------------
static void SetSimpleOptions(OnigOptionType& onigOptions, EOLmode eolMode,
  const bool caseSensitive, const bool forwardSearch, const int searchFlags = 0)
{
  // fixed options
  onigOptions = ONIG_OPTION_DEFAULT;

  // Notepad3 forced options
  ONIG_OPTION_OFF(onigOptions, ONIG_OPTION_EXTEND);
  ONIG_OPTION_OFF(onigOptions, ONIG_OPTION_SINGLELINE);
  ONIG_OPTION_ON(onigOptions, ONIG_OPTION_NEGATE_SINGLELINE);
  ONIG_OPTION_OFF(onigOptions, ONIG_OPTION_FIND_LONGEST);

  //ONIG_OPTION_OFF(onigOptions, ONIG_OPTION_ASCII_RANGE);
  //ONIG_OPTION_OFF(onigOptions, ONIG_OPTION_CAPTURE_GROUP);

  // dynamic options

  //switch (eolMode) {
  //  case EOLmode::CR:
  //  case EOLmode::LF:
  //  case EOLmode::CRLF:
  //  default:
  //    break;
  //}

  if (searchFlags & SCFIND_DOT_MATCH_ALL) {
    ONIG_OPTION_ON(onigOptions, ONIG_SYN_OP_DOT_ANYCHAR);
    ONIG_OPTION_ON(onigOptions, ONIG_OPTION_MULTILINE);
  }
  else {
    ONIG_OPTION_OFF(onigOptions, ONIG_SYN_OP_DOT_ANYCHAR);
    ONIG_OPTION_OFF(onigOptions, ONIG_OPTION_MULTILINE);
  }

  if (caseSensitive) {
    ONIG_OPTION_OFF(onigOptions, ONIG_OPTION_IGNORECASE);
  }
  else {
    ONIG_OPTION_ON(onigOptions, ONIG_OPTION_IGNORECASE);
  }

  if (forwardSearch) {
  }
  else {
  }
}
// ============================================================================





class OnigurumaRegExEngine : public RegexSearchBase
{
public:

  explicit OnigurumaRegExEngine(CharClassify* charClassTable)
    : m_OnigSyntax(*ONIG_SYNTAX_DEFAULT)
    , m_CmplOptions(ONIG_OPTION_DEFAULT)
    , m_RegExpr(nullptr)
    , m_Region({0,0,nullptr,nullptr,nullptr})
    , m_ErrorInfo()
    , m_MatchPos(ONIG_MISMATCH)
    , m_MatchLen(0)
  {
    m_OnigSyntax.op |= ONIG_SYN_OP_ESC_LTGT_WORD_BEGIN_END; // xcluded from ONIG_SYNTAX_DEFAULT ?
    onig_initialize(s_UsedEncodingsTypes, _ARRAYSIZE(s_UsedEncodingsTypes));
    onig_region_init(&m_Region);
  }

  ~OnigurumaRegExEngine() override
  {
    onig_region_free(&m_Region, 0); /* 1:free self, 0:free contents only */
    onig_free(m_RegExpr);
    onig_end();
  }

  Sci::Position FindText(Document* doc, Sci::Position minPos, Sci::Position maxPos, const char* pattern,
                         bool caseSensitive, bool word, bool wordStart, int searchFlags, Sci::Position* length) override;

  const char* SubstituteByPosition(Document* doc, const char* text, Sci::Position* length) override;

  const OnigRegion& GetRegion() const { return m_Region; };

private:

  std::string& translateRegExpr(std::string& regExprStr, bool wholeWord, bool wordStart, int eolMode, OnigOptionType& rxOptions);

  std::string& convertReplExpr(std::string& replStr);

  //void regexFindAndReplace(std::string& inputStr_inout, const std::string& patternStr, const std::string& replStr);

private:

  std::string m_RegExprStrg;

  OnigSyntaxType  m_OnigSyntax;
  OnigOptionType  m_CmplOptions;
  OnigRegex       m_RegExpr;
  OnigRegion      m_Region;

  char            m_ErrorInfo[ONIG_MAX_ERROR_MESSAGE_LEN];

  Sci::Position   m_MatchPos;
  Sci::Position   m_MatchLen;

public:
  std::string m_SubstBuffer;

};
// ============================================================================


RegexSearchBase *Scintilla::CreateRegexSearch(CharClassify *charClassTable)
{
  return new OnigurumaRegExEngine(charClassTable);
}

// ============================================================================



// ============================================================================
//   Some Helpers
// ============================================================================


/******************************************************************************
*
*  UnSlash functions
*  Mostly taken from SciTE, (c) Neil Hodgson, http://www.scintilla.org
*
* Is the character an octal digit?
*/
constexpr bool IsOctalDigit(char ch)
{
  return ((ch >= '0') && (ch <= '7'));
}
// ----------------------------------------------------------------------------

/**
* If the character is an hex digit, get its value.
*/
constexpr int GetHexDigit(char ch)
{
  if ((ch >= '0') && (ch <= '9')) {
    return (ch - '0');
  }
  if ((ch >= 'A') && (ch <= 'F')) {
    return (ch - 'A' + 10);
  }
  if ((ch >= 'a') && (ch <= 'f')) {
    return (ch - 'a' + 10);
  }
  return -1;
}
// ----------------------------------------------------------------------------


static void replaceAll(std::string& source, const std::string& from, const std::string& to)
{
  std::string newString;
  newString.reserve(source.length() * 2);  // avoids a few memory allocations

  std::string::size_type lastPos = 0;
  std::string::size_type findPos;

  while (std::string::npos != (findPos = source.find(from, lastPos))) {
    newString.append(source, lastPos, findPos - lastPos);
    newString += to;
    lastPos = findPos + from.length();
  }
  // Care for the rest after last occurrence
  newString += source.substr(lastPos);

  source.swap(newString);
}
// ----------------------------------------------------------------------------


/**
 * Find text in document, supporting both forward and backward
 * searches (just pass minPos > maxPos to do a backward search)
 * Has not been tested with backwards DBCS searches yet.
 */
Sci::Position OnigurumaRegExEngine::FindText(Document* doc, Sci::Position minPos, Sci::Position maxPos, const char *pattern,
                                 bool caseSensitive, bool word, bool wordStart, int searchFlags, Sci::Position *length)
{
  if (!(pattern && (strlen(pattern) > 0))) {
    *length = 0;
    return SciPos(-1);
  }

  auto const docLen = SciPos(doc->Length());
  EOLmode const eolMode = static_cast<EOLmode>(doc->eolMode);

  bool const findForward = (minPos <= maxPos);
  int const increment = findForward ? 1 : -1;

  // Range endpoints should not be inside DBCS characters, but just in case, move them.
  minPos = doc->MovePositionOutsideChar(minPos, increment, false);
  maxPos = doc->MovePositionOutsideChar(maxPos, increment, false);
  
  if (!findForward) {
    minPos = doc->MovePositionOutsideChar(minPos - 1, increment, false);
  }


  Sci::Position const rangeBeg = (findForward) ? minPos : maxPos;
  Sci::Position const rangeEnd = (findForward) ? maxPos : minPos;
  Sci::Position const rangeLen = (rangeEnd - rangeBeg);

  OnigOptionType onigOptions;
  SetSimpleOptions(onigOptions, eolMode, caseSensitive, findForward, searchFlags);
  ONIG_OPTION_ON(onigOptions, (rangeBeg > 0) ? ONIG_OPTION_NOTBOL : ONIG_OPTION_NONE);
  ONIG_OPTION_ON(onigOptions, (rangeEnd < docLen) ? ONIG_OPTION_NOTEOL : ONIG_OPTION_NONE);
  
  std::string sPattern(pattern);
  std::string const & sRegExprStrg = translateRegExpr(sPattern, word, wordStart, doc->eolMode, onigOptions);

  bool const bReCompile = (m_RegExpr == nullptr) || (m_CmplOptions != onigOptions) || (m_RegExprStrg.compare(sRegExprStrg) != 0);

  if (bReCompile) {
    m_RegExprStrg.clear();
    m_RegExprStrg = sRegExprStrg;
    m_CmplOptions = onigOptions;
    m_ErrorInfo[0] = '\0';
    try {
      OnigErrorInfo einfo;
      onig_free(m_RegExpr);

      //OnigEncoding const onigEncType = ONIG_ENCODING_UTF8;
      OnigEncoding const onigEncType = (eolMode == EOLmode::LF) ? ONIG_ENCODING_UTF8 : 
                                      ((eolMode == EOLmode::CR) ? ONIG_ENCODING_UTF8_CR : ONIG_ENCODING_UTF8_CRLF);
      int res = onig_new(&m_RegExpr, UCharCPtr(m_RegExprStrg.c_str()), UCharCPtr(m_RegExprStrg.c_str() + m_RegExprStrg.length()),
                         m_CmplOptions, onigEncType, &m_OnigSyntax, &einfo);
      if (res != ONIG_NORMAL) {
        onig_error_code_to_str(UCharPtr(m_ErrorInfo), res, &einfo);
        return SciPos(-2);   // -1 is normally used for not found, -2 is used here for invalid regex
      }
    }
    catch (...) {
      return SciPos(-2);
    }
  }

  m_MatchPos = SciPos(ONIG_MISMATCH); // not found
  m_MatchLen = SciPos(0);

  // ---  search document range for pattern match   ---
  // !!! Performance issue: Scintilla: moving Gap needs memcopy - high costs for find/replace in large document
  auto const docBegPtr = UCharCPtr(doc->RangePointer(0, docLen));
  auto const docEndPtr = UCharCPtr(doc->RangePointer(docLen, 0));
  auto const rangeBegPtr = UCharCPtr(doc->RangePointer(rangeBeg, rangeLen));
  auto const rangeEndPtr = UCharCPtr(doc->RangePointer(rangeEnd, 0));

  OnigPosition result = ONIG_MISMATCH;
  try {
    onig_region_free(&m_Region, 0);  /* 1:free self, 0:free contents only */
    onig_region_init(&m_Region);

    if (findForward)
      result = onig_search(m_RegExpr, docBegPtr, docEndPtr, rangeBegPtr, rangeEndPtr, &m_Region, onigOptions);
    else //                                                              X                                    //
      result = onig_search(m_RegExpr, docBegPtr, docEndPtr, rangeEndPtr, rangeBegPtr, &m_Region, onigOptions);
  }
  catch (...) {
    return SciPos(-3);  // -1 is normally used for not found, -3 is used here for exception
  }

  if (result < ONIG_MISMATCH) {
    onig_error_code_to_str(UCharPtr(m_ErrorInfo), result);
    return SciPos(-3);
  }

  if ((result >= 0) && (rangeBegPtr <= rangeEndPtr)) 
  {
    //~m_MatchPos = SciPos(result); //
    m_MatchPos = SciPos(m_Region.beg[0]);
    //~m_MatchLen = SciPos(m_Region.end[0] - result);
    m_MatchLen = SciPos(m_Region.end[0]) - SciPos(m_Region.beg[0]);
  }

  //NOTE: potential 64-bit-size issue at interface here:
  *length = m_MatchLen;
  return m_MatchPos;
}
// ============================================================================



////static int GrpNameCallback(const UChar* name, const UChar* name_end, 
////  int ngroup_num, int* group_nums, regex_t* reg, void* arg)
////{
////  OnigurumaRegExEngine* pRegExInstance = dynamic_cast<OnigurumaRegExEngine*>(arg);
////
////  const OnigRegion& region = pRegExInstance->GetRegion();
////
////  for (int i = 0; i < ngroup_num; i++) 
////  {
////    int grpNum = group_nums[i];
////    
////    int ref = onig_name_to_backref_number(reg, name, name_end, &region);
////
////    if (ref == grpNum) {
////
////      Sci::Position rBeg = SciPos(region.beg[grpNum]);
////      Sci::Position len = SciPos(region.end[grpNum] - rBeg);
////
////      (pRegExInstance->m_SubstBuffer).append(doc->RangePointer(rBeg, len), (size_t)len);
////
////    }
////  }
////  return 0;  /* 0: continue */
////}
////   called by:  int r = onig_foreach_name(m_RegExpr, GrpNameCallback, (void*)this);


// ============================================================================


const char* OnigurumaRegExEngine::SubstituteByPosition(Document* doc, const char* text, Sci::Position* length)
{
  if (m_MatchPos < 0) {
    *length = SciPos(-1);
    return nullptr;
  }
  std::string sText(text, *length);
  std::string const & rawReplStrg = convertReplExpr(sText);

  m_SubstBuffer.clear();

  for (size_t j = 0; j < rawReplStrg.length(); ++j) 
  {
    bool bReplaced = false;
    if ((rawReplStrg[j] == '$') || (rawReplStrg[j] == '\\'))
    {
      if ((rawReplStrg[j + 1] >= '0') && (rawReplStrg[j + 1] <= '9'))
      {
        // group # limit = 99 / TODO: allow for arbitrary number of groups/regions

        bool const digit2nd = ((rawReplStrg[j + 2] >= '0') && (rawReplStrg[j + 2] <= '9')) && (m_Region.num_regs > 10);
        int const grpNum = digit2nd ? (rawReplStrg[j + 1] - '0') * 10 + (rawReplStrg[j + 2] - '0') : (rawReplStrg[j + 1] - '0');
        if (grpNum < m_Region.num_regs)
        {
          auto const rBeg = SciPos(m_Region.beg[grpNum]);
          auto const  len = SciPos(m_Region.end[grpNum] - rBeg);

          m_SubstBuffer.append(doc->RangePointer(rBeg, len), static_cast<size_t>(len));
        }
        bReplaced = true;
        j += digit2nd ? 2 : 1;
      }
      else if (rawReplStrg[j] == '$')
      {
        size_t k = ((rawReplStrg[j + 1] == '+') && (rawReplStrg[j + 2] == '{')) ? (j + 3) : ((rawReplStrg[j + 1] == '{') ? (j + 2) : 0);
        if (k > 0) {
          // named group replacement
          auto const name_beg = UCharCPtr(&(rawReplStrg[k]));
          while (rawReplStrg[k] &&  IsCharAlphaNumericA(rawReplStrg[k])) { ++k; }
          if (rawReplStrg[k] == '}')
          {
            int const grpNum = onig_name_to_backref_number(m_RegExpr, name_beg, UCharCPtr(&(rawReplStrg[k])), &m_Region);
            if ((grpNum >= 0) && (grpNum < m_Region.num_regs))
            {
              auto const rBeg = SciPos(m_Region.beg[grpNum]);
              auto const  len = SciPos(m_Region.end[grpNum] - rBeg);

              m_SubstBuffer.append(doc->RangePointer(rBeg, len), static_cast<size_t>(len));
            }
            bReplaced = true;
            j = k;
          }
        }
      }
      else if ((rawReplStrg[j + 1] == '$') || (rawReplStrg[j + 1] == '\\')) {
        ++j; //  '\$' -> '$' or '\\' -> '\'
      }
    }
    if (!bReplaced) { m_SubstBuffer.push_back(rawReplStrg[j]); }
  }

  //NOTE: potential 64-bit-size issue at interface here:
  *length = SciPos(m_SubstBuffer.length());
  return m_SubstBuffer.c_str();
}
// ============================================================================



// ============================================================================
//
// private methods
//
// ============================================================================

/*
void OnigurumaRegExEngine::regexFindAndReplace(std::string& inputStr_inout, const std::string& patternStr, const std::string& replStr)
{
  OnigRegex       oRegExpr;
  OnigRegion      oRegion;

  const UChar* pattern = (UChar*)patternStr.c_str();

  OnigErrorInfo einfo;
  int res = onig_new(&oRegExpr, pattern, pattern + strlen((char*)pattern),
    ONIG_OPTION_DEFAULT, ONIG_ENCODING_ASCII, ONIG_SYNTAX_DEFAULT, &einfo);

  if (res != ONIG_NORMAL) { return; }
  
  const UChar* strg = (UChar*)inputStr_inout.c_str();
  const UChar* start = strg;
  const UChar* end = (start + patternStr.length());
  const UChar* range = end;

  onig_region_init(&oRegion);

  OnigPosition pos = onig_search(oRegExpr, strg, end, start, range, &oRegion, ONIG_OPTION_DEFAULT);

  if (pos >= 0) 
  {
    std::string replace = replStr; // copy
    for (int i = 1; i < oRegion.num_regs; i++) {
      std::ostringstream nr;
      nr << R"(\)" << i;
      std::string regio((char*)(strg + oRegion.beg[i]), (oRegion.end[i] - oRegion.beg[i]));
      replaceAll(replace, nr.str(), regio);
    }
    inputStr_inout.replace(oRegion.beg[0], (oRegion.end[0] - oRegion.beg[0]), replace);
  }

  onig_region_free(&oRegion, 0);  // 1:free self, 0:free contents only
  onig_free(oRegExpr);
}
// ----------------------------------------------------------------------------
*/



std::string& OnigurumaRegExEngine::translateRegExpr(std::string& regExprStr, bool wholeWord, bool wordStart, int eolMode, OnigOptionType& rxOptions)
{
  std::string	tmpStr;
  bool bUseTmpStrg = false;

  if (wholeWord || wordStart) {      // push '\b' at the begin of regexpr
    tmpStr.push_back('\\');
    tmpStr.push_back('b');
    tmpStr.append(regExprStr);
    if (wholeWord) {               // push '\b' at the end of regexpr
      tmpStr.push_back('\\');
      tmpStr.push_back('b');
    }
    replaceAll(tmpStr, ".", R"(\w)");
    bUseTmpStrg = true;
  }
  else {
    tmpStr.append(regExprStr);
  }

  // Oniguruma supports LTGT word boundary by: ONIG_SYN_OP_ESC_LTGT_WORD_BEGIN_END
  //
  //~replaceAll(tmpStr, R"(\<)", R"((?<!\w)(?=\w))");  // word begin
  //~replaceAll(tmpStr, R"(\(?<!\w)(?=\w))", R"(\\<)"); // esc'd
  //~replaceAll(tmpStr, R"(\>)", R"((?<=\w)(?!\w))"); // word end
  //~replaceAll(tmpStr, R"(\(?<=\w)(?!\w))", R"(\\>)"); // esc'd
  //~bUseTmpStrg = true;


  // EOL modes
  switch (eolMode) {
  case SC_EOL_LF:
  case SC_EOL_CR:
    //ONIG_OPTION_OFF(rxOptions, ONIG_OPTION_CRLF_AS_LINE_SEPARATOR);
    break;

  case SC_EOL_CRLF:
    //ONIG_OPTION_ON(rxOptions, ONIG_OPTION_CRLF_AS_LINE_SEPARATOR);
    break;
  }

  if (bUseTmpStrg) { std::swap(regExprStr, tmpStr); }

  return regExprStr;
}
// ----------------------------------------------------------------------------


std::string& OnigurumaRegExEngine::convertReplExpr(std::string& replStr)
{
  std::string	tmpStr;
  for (size_t i = 0; i < replStr.length(); ++i) {
    char ch = replStr[i];
    if (ch == '\\') {
      ch = replStr[++i]; // next char
      if (ch >= '1' && ch <= '9') {
        // former behavior convenience: 
        // change "\\<n>" to deelx's group reference ($<n>)
        tmpStr.push_back('$');
        tmpStr.push_back(ch);
        continue;
      }
      switch (ch) {
        // check for escape seq:
      case 'a':
        tmpStr.push_back('\a');
        break;
      case 'b':
        tmpStr.push_back('\x1B');
        break;
      case 'f':
        tmpStr.push_back('\f');
        break;
      case 'n':
        tmpStr.push_back('\n');
        break;
      case 'r':
        tmpStr.push_back('\r');
        break;
      case 't':
        tmpStr.push_back('\t');
        break;
      case 'v':
        tmpStr.push_back('\v');
        break;
      case '\\':
        tmpStr.push_back('\\'); // preserve escd "\"
        tmpStr.push_back('\\'); 
        break;
      case 'x':
      case 'u':
        {
          bool bShort = (ch == 'x');
          char buf[8] = { '\0' };
          char *pch = buf;
          WCHAR val[2] = L"";
          int hex;
          val[0] = 0;
          ++i;
          hex = GetHexDigit(replStr[i]);
          if (hex >= 0) {
            ++i;
            val[0] = static_cast<WCHAR>(hex);
            hex = GetHexDigit(replStr[i]);
            if (hex >= 0) {
              ++i;
              val[0] *= 16;
              val[0] += static_cast<WCHAR>(hex);
              if (!bShort) {
                hex = GetHexDigit(replStr[i]);
                if (hex >= 0) {
                  ++i;
                  val[0] *= 16;
                  val[0] += static_cast<WCHAR>(hex);
                  hex = GetHexDigit(replStr[i]);
                  if (hex >= 0) {
                    ++i;
                    val[0] *= 16;
                    val[0] += static_cast<WCHAR>(hex);
                  }
                }
              }
            }
            if (val[0]) {
              val[1] = 0;
              WideCharToMultiByte(CP_UTF8, 0, val, -1, buf, ARRAYSIZE(val), nullptr, nullptr);
              tmpStr.push_back(*pch++);
              while (*pch)
                tmpStr.push_back(*pch++);
            }
            else
              tmpStr.push_back(ch); // unknown hex seq
          }
          else
            tmpStr.push_back(ch); // unknown hex seq
        }
        break;

      default: // unknown ctrl seq
        tmpStr.push_back('\\'); // revert
        tmpStr.push_back(ch);
        break;
      }
    }
    else {
      tmpStr.push_back(ch);
    }
  } //for

  std::swap(replStr,tmpStr);
  return replStr;
}
// ============================================================================





// ============================================================================
// ============================================================================


class SimpleRegExEngine
{
public:

  explicit SimpleRegExEngine(const EOLmode eolMode)
    : m_EOLmode(eolMode)
    , m_OnigSyntax(*ONIG_SYNTAX_DEFAULT)
    , m_Options(ONIG_OPTION_DEFAULT)
    , m_RegExpr(nullptr)
    , m_Region({ 0,0,nullptr,nullptr,nullptr })
    , m_ErrorInfo()
    , m_MatchPos(ONIG_MISMATCH)
    , m_MatchLen(0)
  {
    m_OnigSyntax.op |= ONIG_SYN_OP_ESC_LTGT_WORD_BEGIN_END; // xcluded from ONIG_SYNTAX_DEFAULT ?
    onig_initialize(s_UsedEncodingsTypes, _ARRAYSIZE(s_UsedEncodingsTypes));
    onig_region_init(&m_Region);
  }

  ~SimpleRegExEngine() noexcept
  {
    onig_region_free(&m_Region, 0);  /* 1:free self, 0:free contents only */
    onig_free(m_RegExpr);
    onig_end();
  }

  OnigPosition Find(const OnigUChar* pattern, const OnigUChar* document, const  bool caseSensitive);

  const OnigPosition GetMatchPos() const { return m_MatchPos; };
  const OnigPosition GetMatchLen() const { return m_MatchLen; };
  const OnigRegion& GetRegion() const { return m_Region; };

private:

  //void regexFindAndReplace(std::string& inputStr_inout, const std::string& patternStr, const std::string& replStr);

private:

  EOLmode         m_EOLmode;
  OnigSyntaxType  m_OnigSyntax;
  OnigOptionType  m_Options;
  OnigRegex       m_RegExpr;
  OnigRegion      m_Region;

  OnigUChar       m_ErrorInfo[ONIG_MAX_ERROR_MESSAGE_LEN];

  OnigPosition    m_MatchPos;
  OnigPosition    m_MatchLen;

};
// ============================================================================


OnigPosition SimpleRegExEngine::Find(const OnigUChar* pattern, const OnigUChar* document, const bool caseSensitive)
{
  auto const patternLen = (pattern) ? OnigPosition(_mbslen(pattern)) : 0;
  if (patternLen == 0) {
    return OnigPosition(-1);
  }

  auto const stringLen = (document) ? OnigPosition(_mbslen(document)) : 0;
  if (stringLen == 0) {
    return OnigPosition(-1);
  }

  // init search options
  SetSimpleOptions(m_Options, m_EOLmode, caseSensitive, true);
  m_ErrorInfo[0] = '\0';

  try {
    onig_free(m_RegExpr);

    //OnigEncoding const onigEncType = ONIG_ENCODING_UTF8;
    OnigEncoding const onigEncType = (m_EOLmode == EOLmode::LF) ? ONIG_ENCODING_UTF8 :
                                    ((m_EOLmode == EOLmode::CR) ? ONIG_ENCODING_UTF8_CR : ONIG_ENCODING_UTF8_CRLF);

    OnigErrorInfo einfo;
    int res = onig_new(&m_RegExpr, pattern, (pattern + patternLen), m_Options, onigEncType, &m_OnigSyntax, &einfo);

    if (res != ONIG_NORMAL) {
      //onig_error_code_to_str(m_ErrorInfo, res, &einfo);
      return OnigPosition(-111);
    }

    onig_region_free(&m_Region, 0);
    onig_region_init(&m_Region);

    const UChar* strgBeg = document;
    const UChar* strgEnd = document + stringLen;
    const UChar* rangeBeg = strgBeg;
    const UChar* rangeEnd = strgEnd;

    // start search
    OnigPosition result = onig_search(m_RegExpr, strgBeg, strgEnd,
      rangeBeg, rangeEnd, &m_Region, m_Options);

    if (result < ONIG_MISMATCH) {
      //onig_error_code_to_str(m_ErrorInfo, result);
      return OnigPosition(-3);
    }

    m_MatchPos = OnigPosition(ONIG_MISMATCH); // not found
    m_MatchLen = OnigPosition(0);

    if (result >= 0) // found
    {
      //~m_MatchPos = result; //
      m_MatchPos = m_Region.beg[0];
      //~m_MatchLen = (m_Region.end[0] - result);
      m_MatchLen = (m_Region.end[0] - m_Region.beg[0]);
    }
    //else if (result == ONIG_MISMATCH) // not found
    //{
    //  m_MatchPos = result;
    //  m_MatchLen = OnigPosition(0);
    //}
    else if (result < ONIG_MISMATCH)
    {
      //onig_error_code_to_str(m_ErrorInfo, result);
      m_MatchPos = result;
      m_MatchLen = OnigPosition(0);
    }
  }
  catch (...) {
    // -1 is normally used for not found, -666 is used here for exception
    return OnigPosition(-666);
  }

  return m_MatchPos;
}
// ============================================================================

extern "C"
#ifdef SCINTILLA_DLL
__declspec(dllexport)
#endif
ptrdiff_t WINAPI OnigRegExFind(const char* pchPattern, const char* pchText, const bool caseSensitive, const int eolMode)
{
  const UChar* pattern = reinterpret_cast<const UChar*>(pchPattern);
  const UChar* string = reinterpret_cast<const UChar*>(pchText);

  SimpleRegExEngine ModuleRegExEngine(static_cast<EOLmode>(eolMode));

  return static_cast<ptrdiff_t>(ModuleRegExEngine.Find(pattern, string, caseSensitive));
}
// ============================================================================

#endif //SCI_OWNREGEX
