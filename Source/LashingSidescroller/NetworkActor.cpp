// Fill out your copyright notice in the Description page of Project Settings.

#include "LashingSidescroller.h"
#include "NetworkActor.h"
#include "SocketSubsystem.h"
#include "IPv4SubnetMask.h"
#include "IPv4Address.h"
#include "IPv4Endpoint.h"
#include "TcpSocketBuilder.h"
#include "UnrealMathUtility.h"

// Sets default values
ANetworkActor::ANetworkActor() :
	Socket(nullptr)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}



ANetworkActor::~ANetworkActor()
{
	if(Connection != nullptr)
	{
		Connection->Close();

		delete Connection;
	}

	if(Socket != nullptr)
	{
		Socket->Close();
	
		delete Socket;
	}
}

// Called when the game starts or when spawned
void ANetworkActor::BeginPlay()
{
	Super::BeginPlay();
	
	FString address = TEXT("0.0.0.0");
	int32 port = 19834;

	bool valid = true;
	
	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	addr->SetIp(*address, valid);
	addr->SetPort(port);

	FIPv4Endpoint Endpoint(addr);
	
	Socket = FTcpSocketBuilder(TEXT("default")).AsReusable().BoundToEndpoint(Endpoint).Listening(8);

	int32 new_size;
	Socket->SetReceiveBufferSize(2 << 20, new_size);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Connection ~> %d"), Socket != nullptr));
	
	GetWorldTimerManager().SetTimer(this, 
		&ANetworkActor::TCPConnectionListener, 0.01, true);//*/
}

// Called every frame
void ANetworkActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

void ANetworkActor::TCPConnectionListener()
{
	if(!Socket) return;
 
	//Remote address
	RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool Pending;
 
	// handle incoming connections
	if (Socket->HasPendingConnection(Pending) && Pending)
	{
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//Already have a Connection? destroy previous
		if(Connection)
		{
			Connection->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Connection);
		}
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 
		//New Connection receive!
		Connection = Socket->Accept(*RemoteAddress, TEXT("connection"));
 
		if (Connection != NULL)
		{
			//can thread this too
			GetWorldTimerManager().SetTimer(this, 
				&ANetworkActor::TCPSocketListener, 0.01, true);	
		}
	}
}

void ANetworkActor::TCPSocketListener()
{
	if(!Connection) return;
 
	TArray<uint8> ReceivedData;
 
	uint32 Size;
	while (Connection->HasPendingData(Size))
	{
		ReceivedData.SetNumUninitialized(FMath::Min(Size, 65507u));
 
		int32 Read = 0;
		Connection->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);
	}
 
	if(ReceivedData.Num() <= 0)
	{
		//No Data Received
		return;
	}

	int length = ReceivedData.Num() / sizeof(float);
 
	const float* angles = reinterpret_cast<const float*>(ReceivedData.GetData());

	FVector Euler = FVector(FMath::RadiansToDegrees(angles[length - 3]), FMath::RadiansToDegrees(angles[length - 2]), FMath::RadiansToDegrees(angles[length - 1]));

	Rotator = FRotator(-Euler.Y, Euler.X, -Euler.Z);
	
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Data Bytes Read ~> %d - %f %f %f"), length, Euler.X, Euler.Y, Euler.Z));
}

