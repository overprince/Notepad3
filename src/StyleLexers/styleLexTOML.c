#include "StyleLexers.h"

// ----------------------------------------------------------------------------

//KEYWORDLIST KeyWords_TOML = EMPTY_KEYWORDLIST;
KEYWORDLIST KeyWords_TOML =
{
    "+inf +nan -inf -nan false inf nan true", // Keyword
    NULL,
};


EDITLEXER lexTOML =
{
    SCLEX_TOML, "TOML", IDS_LEX_TOML_CFG, L"TOML Config", L"toml", L"",
    &KeyWords_TOML,{
        { {STYLE_DEFAULT}, IDS_LEX_STR_63126, L"Default", L"", L"" },
        //{ {SCE_TOML_DEFAULT}, IDS_LEX_STR_63126, L"Default", L"", L"" },
        { {SCE_TOML_KEYWORD}, IDS_LEX_STR_63128, L"Keyword", L"bold; fore:#FF0080", L"" },
        { {SCE_TOML_COMMENT}, IDS_LEX_STR_63127, L"Comment", L"fore:#008000", L"" },
        { {SCE_TOML_SECTION}, IDS_LEX_STR_63232, L"Section", L"bold; fore:#000000; back:#FFF1A8; eolfilled", L"" },
        { {SCE_TOML_KEY}, IDS_LEX_STR_63348, L"Key", L"bold; fore:#5E608F", L"" },
        { {SCE_TOML_ASSIGNMENT}, IDS_LEX_STR_63233, L"Assignment", L"bold; fore:#FF2020", L"" },
        { {SCE_TOML_VALUE}, IDS_LEX_STR_63201, L"Value", L"fore:#202020", L"" },
        { {SCE_TOML_NUMBER}, IDS_LEX_STR_63130, L"Number", L"fore:#0000E0", L"" },
        { {SCE_TOML_DATETIME}, IDS_LEX_STR_63356, L"Date-Time", L"fore:#950095", L"" },
        { {MULTI_STYLE(SCE_TOML_STR_BASIC, SCE_TOML_STR_LITERAL,0,0)}, IDS_LEX_STR_63131, L"String", L"italic; fore:#606060", L"" },
        { {SCE_TOML_PARSINGERROR}, IDS_LEX_STR_63252, L"Parsing Error", L"fore:#FFFF00; back:#A00000; eolfilled", L"" },
        EDITLEXER_SENTINEL
    }
};
