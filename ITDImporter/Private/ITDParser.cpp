#include "ITDImporter/Public/ITDParser.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

UITDParser::UITDParser() {
    UE_LOG(LogTemp, Warning, TEXT("UITDParser Constructor called"));
}

UITDParser::~UITDParser() {
    UE_LOG(LogTemp, Warning, TEXT("UITDParser Destructor called"));
}

bool UITDParser::ParseFile(const FString& FilePath) {
    FString FileContent;
    if (!FFileHelper::LoadFileToString(FileContent, *FilePath)) {
        UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *FilePath);
        return false;
    }

    TArray<FString> Lines;
    FileContent.ParseIntoArrayLines(Lines);

    ParseLines(Lines);

    return true;
}

// Lines �迭�� ����Ͽ� Polygons ������ ����
void UITDParser::ParseLines(const TArray<FString>& Lines) {
    int32 LineIndex = 0;
    while (LineIndex < Lines.Num()) {
        FString Line = Lines[LineIndex].TrimStartAndEnd();

        if (Line.StartsWith(TEXT("[POLYGON"))) {
            ParsePolygon(Lines, LineIndex);
            UE_LOG(LogTemp, Warning, TEXT("**********************StartPasing Polygon******************"));
        }
        else {
            LineIndex++;
        }
    }
}

void UITDParser::ParsePolygon(const TArray<FString>& Lines, int32& LineIndex) {
    // POLYGON ���� �Ľ�
    FString Line = Lines[LineIndex].TrimStartAndEnd();

    FITDPolygon NewPolygon;

    // ���ؽ� �� �� PLANE �Ľ�
    // ��: "POLYGON [PLANE 0 0 1 -0.006485] 3"
    TArray<FString> Tokens;
    Line.ParseIntoArrayWS(Tokens);

    int32 VertexCount = 0;

    int32 PlaneStartIndex = Line.Find(TEXT("[PLANE"));
    if (PlaneStartIndex != INDEX_NONE)
    {
        int32 PlaneEndIndex = Line.Find(TEXT("]"), ESearchCase::IgnoreCase, ESearchDir::FromStart, PlaneStartIndex);
        if (PlaneEndIndex != INDEX_NONE)
        {
	    // PLANE �Ľ�
            FString PlaneData = Line.Mid(PlaneStartIndex + 1, PlaneEndIndex - PlaneStartIndex - 1);
            PlaneData.RemoveFromStart(TEXT("PLANE "));
            TArray<FString> PlaneComponents;
            PlaneData.ParseIntoArrayWS(PlaneComponents);;

	    // ��� ������ �Ľ�
            if (PlaneComponents.Num() == 4) {
                NewPolygon.PlaneNormal.X = FCString::Atod(*PlaneComponents[0]);
                NewPolygon.PlaneNormal.Y = FCString::Atod(*PlaneComponents[1]);
                NewPolygon.PlaneNormal.Z = FCString::Atod(*PlaneComponents[2]);
                NewPolygon.PlaneDistance = FCString::Atod(*PlaneComponents[3]);
                NewPolygon.bHasPlane = true;

                
            }
	    // ��� �����Ͱ� 4���� �ƴ� ��� ��� ���
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Invalid plane data: %s"), *PlaneData);
            }
        }
	// ��� �������� ���� ã�� ���� ��� ��� ���
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to find end of plane data: %s"), *Line);
            NewPolygon.bHasPlane = false;
        }
    }

    // ���ؽ� �� �Ľ� (������ ��ū)
    if (Tokens.Num() >= 2) {
        FString LastToken = Tokens.Last();
        VertexCount = FCString::Atoi(*LastToken);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to parse vertex count: %s"), *Line);
        return;
    }

    LineIndex++; // ���� �������� �̵�

    // ���ؽ� �Ľ�
    for (int32 i = 0; i < VertexCount; ++i) {
        if (LineIndex >= Lines.Num()) {
            UE_LOG(LogTemp, Warning, TEXT("Expected more lines for vertices, but reached end of file."));
            break;
        }

        Line = Lines[LineIndex].TrimStartAndEnd();
        

        FITDVertex NewVertex;

        if (Line.StartsWith(TEXT("[[NORMAL"))) {
            // ��ְ� ��ġ ��ǥ �Ľ�
            // ��: "[[NORMAL nx ny nz] x y z]"
            UE_LOG(LogTemp, Log, TEXT("Vertex with normal detected."));
            FString Content = Line.Mid(2, Line.Len() - 3); // ó���� "[["�� ���� "]" ����
            TArray<FString> VertexTokens;
            Content.ParseIntoArrayWS(VertexTokens);

            if (VertexTokens.Num() >= 7 && VertexTokens[0] == TEXT("NORMAL")) {
                // ��� �Ľ�
                NewVertex.Normal.X = FCString::Atod(*VertexTokens[1]);
                NewVertex.Normal.Y = FCString::Atod(*VertexTokens[2]);
                NewVertex.Normal.Z = FCString::Atod(*VertexTokens[3]);
                NewVertex.bHasNormal = true;

                // ��ġ ��ǥ �Ľ�
                NewVertex.Position.X = FCString::Atod(*VertexTokens[4]);
                NewVertex.Position.Y = FCString::Atod(*VertexTokens[5]);
                NewVertex.Position.Z = FCString::Atod(*VertexTokens[6]);
            }
            else {
                UE_LOG(LogTemp, Warning, TEXT("Invalid vertex format with NORMAL: %s"), *Line);
            }
        }
        else if (Line.StartsWith(TEXT("[")) && Line.EndsWith(TEXT("]"))) {
            // ��ġ ��ǥ�� �ִ� ���
            FString Content = Line.Mid(2, Line.Len() - 4); // "["�� "]" ����
            TArray<FString> VertexTokens;
            Content.ParseIntoArrayWS(VertexTokens);

            if (VertexTokens.Num() >= 3) {
                // ��ġ ��ǥ �Ľ�
                NewVertex.Position.X = FCString::Atod(*VertexTokens[0]);
                NewVertex.Position.Y = FCString::Atod(*VertexTokens[1]);
                NewVertex.Position.Z = FCString::Atod(*VertexTokens[2]);
            }
            else {
                UE_LOG(LogTemp, Warning, TEXT("Invalid vertex format: %s"), *Line);
            }
        }
        else {
            UE_LOG(LogTemp, Warning, TEXT("Unknown vertex format: %s"), *Line);
            LineIndex++;
            i--; // ���ؽ� ���� ���߱� ���� �ε��� ����
            continue;
        }
        // �Ľ̵� ���ؽ� ���� ���
        UE_LOG(LogTemp, Warning, TEXT("Parsed Vertex Position: (%f, %f, %f)"), NewVertex.Position.X, NewVertex.Position.Y, NewVertex.Position.Z);
        if (NewVertex.bHasNormal) {
            UE_LOG(LogTemp, Warning, TEXT("Parsed Vertex Normal: (%f, %f, %f)"), NewVertex.Normal.X, NewVertex.Normal.Y, NewVertex.Normal.Z);
        }

        NewPolygon.Vertices.Add(NewVertex);
        UE_LOG(LogTemp, Warning, TEXT("Parsed Vertex: Position=(%f, %f, %f), HasNormal=%d"), NewVertex.Position.X, NewVertex.Position.Y, NewVertex.Position.Z, NewVertex.bHasNormal);
        LineIndex++;
    }

    // ������ �߰�
    if (NewPolygon.Vertices.Num() > 0) {
        Polygons.Add(NewPolygon);
        UE_LOG(LogTemp, Warning, TEXT("Added Polygon with %d vertices"), NewPolygon.Vertices.Num());
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("Polygon has no vertices and will not be added."));
    }

    // POLYGON ����� ������ �̵� (��: "]")
    while (LineIndex < Lines.Num()) {
        Line = Lines[LineIndex].TrimStartAndEnd();
        if (Line == TEXT("]")) {
            LineIndex++; // "]" ��ŵ
            break;
        }
        else {
            LineIndex++;
        }
    }
}
