' TAGS VB header
' Author: Wraith, 2k5-2k6
' consult "tags-readme.txt" for details

Declare Function TAGS_GetVersion Lib "tags.dll" () As Long
Declare Function TAGS_SetUTF8 Lib "tags.dll" (ByVal enable As Long) As Long
Declare Function TAGS_Read Lib "tags.dll" (ByVal handle As Long, ByVal fmt As String) As Long
Declare Function TAGS_ReadEx Lib "tags.dll" (ByVal handle As Long, ByVal fmt As String, ByVal tagtype As Long, ByVal codepage As Long) As Long
Declare Function TAGS_GetLastErrorDesc Lib "tags.dll" () As Long
