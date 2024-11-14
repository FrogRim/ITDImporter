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
#include "MeshDescription.h"


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
    else {
        UE_LOG(LogTemp, Warning, TEXT("My ITD file: %s"), *FilePath);
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

    // ���� ������ ���� �� �߰�
    TSet<TPair<FVertexID, FVertexID>> ExistingEdges;

    // �����ﺰ�� �޽� ����
    for (const FITDPolygon& PolygonData : Parser->GetPolygons()) 
    {

        TArray<FVertexInstanceID> VertexInstanceIDs;
        //UE_LOG(LogTemp, Warning, TEXT("1. polygon vertice num: %d"), PolygonData.Vertices.Num());

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
                Normals[VertexInstanceID] = FVector3f(0.0f, 0.0f, 1.0f); // �⺻�� ����
            }

            // UV ���� (��ġ ����� ������ UV ����, [0,1] ������ ����ȭ)
            FVector2f UVCoordinate = FVector2f(
                FMath::Fmod(VertexData.Position.X, 1.0f),
                FMath::Fmod(VertexData.Position.Y, 1.0f)
            );
            UVs.Set(VertexInstanceID, 0, UVCoordinate);

            VertexInstanceIDs.Add(VertexInstanceID);
        }

        //UE_LOG(LogTemp, Warning, TEXT("vertex instance num by polygon : %d"), VertexInstanceIDs.Num());
        if (VertexInstanceIDs.Num() >= 3)
        {
            // ���� �ߺ� �˻�
            bool bHasDuplicateEdge = false;
            int32 NumVertices = VertexInstanceIDs.Num();

            for (int32 i = 0; i < NumVertices; ++i)
            {
                FVertexID VertexID0 = MeshDesc->GetVertexInstanceVertex(VertexInstanceIDs[i]);
                FVertexID VertexID1 = MeshDesc->GetVertexInstanceVertex(VertexInstanceIDs[(i + 1) % NumVertices]);

                // ������ ���ؽ� ID�� �����Ͽ� �ߺ� �˻縦 �ϰ��ǰ� ��
                TPair<FVertexID, FVertexID> EdgeKey = TPair<FVertexID, FVertexID>(
                    FMath::Min(VertexID0, VertexID1),
                    FMath::Max(VertexID0, VertexID1)
                );

                if (ExistingEdges.Contains(EdgeKey))
                {
                    UE_LOG(LogTemp, Error, TEXT("Duplicate edge detected between VertexID %d and %d"), VertexID0.GetValue(), VertexID1.GetValue());
                    bHasDuplicateEdge = true;
                    break;
                }
                else
                {
                    ExistingEdges.Add(EdgeKey);
                }
            }

	    if (bHasDuplicateEdge)
	    {
                UE_LOG(LogTemp, Error, TEXT("Cannot create polygon due to duplicate edges."));
                continue; // ���� ���������� �Ѿ
	    }
            
	    // ������ ����
	    MeshDesc->CreatePolygon(PolygonGroupID, VertexInstanceIDs);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Polygon have 3 vertices"));
         
        }

    }

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

    
    if (!MeshDesc->IsEmpty())
    {
        FStaticMeshOperations::ComputeTriangleTangentsAndNormals(*MeshDesc, 0.0f);
    }
    else
    {
	UE_LOG(LogTemp, Error, TEXT("MeshDescription is empty"));
    }

    
    // ��ְ� ź��Ʈ ���
    FStaticMeshOperations::ComputeTangentsAndNormals(
        *MeshDesc,
	EComputeNTBsFlags::Normals | 
        EComputeNTBsFlags::Tangents | 
        EComputeNTBsFlags::UseMikkTSpace |
        EComputeNTBsFlags::WeightedNTBs
    );

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