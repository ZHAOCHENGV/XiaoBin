#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XBSoldierFollowComponent.generated.h"

class AXBCharacterBase;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class XIAOBINDATIANXIA_API UXBSoldierFollowComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UXBSoldierFollowComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
        FActorComponentTickFunction* ThisTickFunction) override;

    // ============================================
    // 主将和目标设置
    // ============================================
    
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetLeader(AXBCharacterBase* NewLeader);

    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetFollowTarget(AActor* NewTarget);

    // ============================================
    // 阵型设置
    // ============================================
    
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetFormationOffset(const FVector& Offset);

    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetFormationSlotIndex(int32 SlotIndex);

    // ============================================
    // 速度设置
    // ============================================
    
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetInterpSpeed(float NewSpeed);

    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetFollowInterpSpeed(float NewSpeed);

    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetFollowSpeed(float NewSpeed);

    // ============================================
    // 跟随控制
    // ============================================
    
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void StartFollowing();

    UFUNCTION(BlueprintCallable, Category = "Follow")
    void StopFollowing();

    UFUNCTION(BlueprintCallable, Category = "Follow")
    void UpdateFollowing(float DeltaTime);

    // ============================================
    // 状态查询
    // ============================================
    
    UFUNCTION(BlueprintCallable, Category = "Follow")
    bool IsAtFormationPosition() const;

    UFUNCTION(BlueprintCallable, Category = "Follow")
    bool IsFollowing() const { return bIsFollowing; }

    UFUNCTION(BlueprintCallable, Category = "Follow")
    FVector GetTargetPosition() const;

protected:
    virtual void BeginPlay() override;

    void UpdateFollowMovement(float DeltaTime);
    FVector CalculateTargetPosition() const;

protected:
    UPROPERTY()
    TWeakObjectPtr<AXBCharacterBase> LeaderRef;

    UPROPERTY()
    TWeakObjectPtr<AActor> FollowTargetRef;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
    FVector FormationOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
    int32 FormationSlotIndex = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
    float InterpSpeed = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
    float FollowSpeed = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
    float MinDistanceToMove = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
    float ArrivalThreshold = 50.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Follow")
    bool bIsFollowing = false;

    UPROPERTY(BlueprintReadOnly, Category = "Follow")
    FVector CachedTargetPosition = FVector::ZeroVector;
};
