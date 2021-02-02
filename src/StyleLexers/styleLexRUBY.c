#include "StyleLexers.h"

// ----------------------------------------------------------------------------

KEYWORDLIST KeyWords_RUBY =
{
    "__FILE__ __LINE__ alias and begin break case class def defined? do else elsif end ensure false for if in "
    "module next nil not or redo rescue retry return self super then true undef unless until when while yield",
    NULL,
};

EDITLEXER lexRUBY =
{
    SCLEX_RUBY, "ruby", IDS_LEX_RUBY, L"Ruby Script", L"rb; ruby; rbw; rake; rjs; rakefile; gemspec; podspec; \\^Rakefile$; \\^Podfile$", L"",
    &KeyWords_RUBY, {
        { {STYLE_DEFAULT}, IDS_LEX_STR_63126, L"Default", L"", L"" },
        //{ {SCE_RB_DEFAULT}, IDS_LEX_STR_63126, L"Default", L"", L"" },
        { {MULTI_STYLE(SCE_RB_COMMENTLINE,SCE_P_COMMENTBLOCK,0,0)}, IDS_LEX_STR_63127, L"Comment", L"fore:#008000", L"" },
        { {SCE_RB_WORD}, IDS_LEX_STR_63128, L"Keyword", L"fore:#00007F", L"" },
        { {SCE_RB_IDENTIFIER}, IDS_LEX_STR_63129, L"Identifier", L"", L"" },
        { {SCE_RB_NUMBER}, IDS_LEX_STR_63130, L"Number", L"fore:#008080", L"" },
        { {SCE_RB_OPERATOR}, IDS_LEX_STR_63132, L"Operator", L"", L"" },
        { {MULTI_STYLE(SCE_RB_STRING,SCE_RB_CHARACTER,SCE_P_STRINGEOL,0)}, IDS_LEX_STR_63131, L"String", L"fore:#FF8000", L"" },
        { {SCE_RB_CLASSNAME}, IDS_LEX_STR_63246, L"Class Name", L"fore:#0000FF", L"" },
        { {SCE_RB_DEFNAME}, IDS_LEX_STR_63247, L"Function Name", L"fore:#007F7F", L"" },
        { {SCE_RB_POD}, IDS_LEX_STR_63292, L"POD", L"fore:#004000; back:#C0FFC0; eolfilled", L"" },
        { {SCE_RB_REGEX}, IDS_LEX_STR_63135, L"Regex", L"fore:#000000; back:#A0FFA0", L"" },
        { {SCE_RB_SYMBOL}, IDS_LEX_STR_63293, L"Symbol", L"fore:#C0A030", L"" },
        { {SCE_RB_MODULE_NAME}, IDS_LEX_STR_63294, L"Module Name", L"fore:#A000A0", L"" },
        { {SCE_RB_INSTANCE_VAR}, IDS_LEX_STR_63295, L"Instance Var", L"fore:#B00080", L"" },
        { {SCE_RB_CLASS_VAR}, IDS_LEX_STR_63296, L"Class Var", L"fore:#8000B0", L"" },
        { {SCE_RB_DATASECTION}, IDS_LEX_STR_63222, L"Data Section", L"fore:#600000; back:#FFF0D8; eolfilled", L"" },
        EDITLEXER_SENTINEL
    }
};
