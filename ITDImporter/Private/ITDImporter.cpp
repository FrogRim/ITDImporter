#include "ITDImporter/Public/ITDImporter.h"
#include "ITDImporter/Public/ITDParser.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Runtime/AssetRegistry/Public/AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "StaticMeshOperations.h"
#include "StaticMeshAttributes.h"


UStaticMesh* UITDImporter::ImportITD(const FString& FilePath, const FString& MeshName, UObject* InParent, EObjectFlags Flags) {
    // ITDParser�� ����Ͽ� ���� �Ľ�
    UITDParser* Parser = NewObject<UITDParser>(InParent);
    if (!Parser) {
        UE_LOG(LogTemp, Error, TEXT("Failed to create ITDParser object"));
        return nullptr;
    }

    // ITD ���� �Ľ�
    if (!Parser->ParseFile(FilePath)) {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse ITD file: %s"), *FilePath);
        return nullptr;
    }

    // StaticMesh ����
    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(InParent, *MeshName, Flags);
    if (!StaticMesh) {
        UE_LOG(LogTemp, Error, TEXT("Failed to create StaticMesh object"));
        Parser->RemoveFromRoot(); // StaticMesh ���� ���� �� ����
        return nullptr;
    }

    // StaticMesh ����
    FStaticMeshSourceModel& SourceModel = StaticMesh->AddSourceModel();
    SourceModel.BuildSettings.bRecomputeNormals = false;
    SourceModel.BuildSettings.bRecomputeTangents = false;

    // MeshDescription ���� �� ����
    FMeshDescription* MeshDesc = StaticMesh->CreateMeshDescription(0);
    if (!MeshDesc) {
        UE_LOG(LogTemp, Error, TEXT("Failed to create MeshDescription for StaticMesh."));
        Parser->RemoveFromRoot(); // MeshDescription ���� ���� �� ����
        return nullptr;
    }

    // MeshDescription �ʱ�ȭ
    *MeshDesc = FMeshDescription();

    // ��Ʈ����Ʈ ��� �� �ʱ�ȭ
    FStaticMeshAttributes MeshAttributes(*MeshDesc);
    MeshAttributes.Register();

    // ��Ʈ����Ʈ ���� ���� (���ؽ�, ���ؽ� �ν��Ͻ�, ������ �׷�)
    TVertexAttributesRef<FVector3f> VertexPositions = MeshAttributes.GetVertexPositions();
    TVertexInstanceAttributesRef<FVector3f> Normals = MeshAttributes.GetVertexInstanceNormals();
    TVertexInstanceAttributesRef<FVector2f> UVs = MeshAttributes.GetVertexInstanceUVs();
    TPolygonGroupAttributesRef<FName> PolygonGroupNames = MeshAttributes.GetPolygonGroupMaterialSlotNames();

    // ������ �׷� ����
    FPolygonGroupID PolygonGroupID = MeshDesc->CreatePolygonGroup();
    PolygonGroupNames[PolygonGroupID] = FName("Default");

    // ���ؽ� ��ġ�� ID ���� -> ���ؽ� �ߺ� ���� ���� �� ����
    TMap<FVector, FVertexID> VertexPositionToIDMap;

    // �����ﺰ�� �޽� ����
    for (const FITDPolygon& PolygonData : Parser->GetPolygons()) 
    {

        TArray<FVertexInstanceID> VertexInstanceIDs;
        UE_LOG(LogTemp, Warning, TEXT("1. polygon vertice num: %d"), PolygonData.Vertices.Num());

        for (const FITDVertex& VertexData : PolygonData.Vertices) 
        {

            // ���ؽ� ����
            FVertexID VertexID;

            if (VertexPositionToIDMap.Contains(VertexData.Position))
            {
                // �̹� ������ ���ؽ� ����
                VertexID = VertexPositionToIDMap[VertexData.Position];
            }
            else
            {
                // ���ؽ� ����
                VertexID = MeshDesc->CreateVertex();
                VertexPositions[VertexID] = FVector3f(VertexData.Position);
                VertexPositionToIDMap.Add(VertexData.Position, VertexID);
            }

            // ���ؽ� �ν��Ͻ� ����
            FVertexInstanceID VertexInstanceID = MeshDesc->CreateVertexInstance(VertexID);

            // ��� ����
            if (VertexData.bHasNormal) 
            {
                Normals[VertexInstanceID] = FVector3f(VertexData.Normal);
            }
            else 
            {
                Normals[VertexInstanceID] = FVector3f::ZeroVector; // �⺻�� ����
            }

            // UV ����
            UVs.Set(VertexInstanceID, 0, FVector2f(0.0f, 0.0f));

            VertexInstanceIDs.Add(VertexInstanceID);
        }

        UE_LOG(LogTemp, Warning, TEXT("vertex instance num by polygon : %d"), VertexInstanceIDs.Num());
        if (VertexInstanceIDs.Num() >=3)
        {
	    // ������ ����
	    MeshDesc->CreatePolygon(PolygonGroupID, VertexInstanceIDs);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Polygon have 3 vertices"));
         
        }
        
    }

    // ��ְ� ź��Ʈ ���
    FStaticMeshOperations::ComputeTangentsAndNormals(
        *MeshDesc,
	EComputeNTBsFlags::Normals | EComputeNTBsFlags::Tangents
    );
    

    // �޽� �������� ��ȿ�� ���� �˻�
    if (MeshDesc->Vertices().Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("MeshDescription have not vertex"));
        return nullptr;
    }

    if (MeshDesc->Polygons().Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("MeshDescription�� have not polygon"));
        return nullptr;
    }

    StaticMesh->CommitMeshDescription(0);

    // StaticMesh ����
    StaticMesh->Build(false);
    StaticMesh->MarkPackageDirty();

    // ���� ���
    FAssetRegistryModule::AssetCreated(StaticMesh);

    Parser = nullptr;

    return StaticMesh;
}

//IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, ITDImporter, "ITDImporter");
IMPLEMENT_MODULE(FDefaultModuleImpl, ITDImporter);