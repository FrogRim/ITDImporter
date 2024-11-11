#include "ITDImporter/Public/ITDFactory.h"
#include "ITDImporter/Public/ITDImporter.h"
#include "Engine/StaticMesh.h"
#include "EditorFramework/AssetImportData.h"


UITDFactory::UITDFactory() {
    bEditorImport = true;
    bText = true;
    Formats.Add(TEXT("itd;ITD Model"));
    SupportedClass = UStaticMesh::StaticClass();
}

UObject* UITDFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName,
    EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn,
    bool& bOutOperationCanceled) {
    // ITDImporter�� ����Ͽ� ���� ����Ʈ
    UITDImporter* Importer = NewObject<UITDImporter>();
    UStaticMesh* StaticMesh = Importer->ImportITD(Filename, InName.ToString(), InParent, Flags);

    if (StaticMesh) {
        // ����Ʈ ������ ����
        StaticMesh->AssetImportData->Update(Filename);

        // ����Ʈ ���� �޽���
        Warn->Logf(ELogVerbosity::Display, TEXT("Successfully imported ITD file: %s"), *Filename);

        return StaticMesh;
    }
    else {
        // ����Ʈ ���� �޽���
        Warn->Logf(ELogVerbosity::Error, TEXT("Failed to import ITD file: %s"), *Filename);
        return nullptr;
    }
}