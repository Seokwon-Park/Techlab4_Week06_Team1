#pragma once

class UObject;

class FArchive
{
public:
	FArchive() = default;
	FArchive(const FArchive&) = default;
	FArchive& operator =(const FArchive& ArchiveToCopy) = default;
	virtual ~FArchive() = default;

	bool IsLoading() const { return bIsLoading; }
	bool IsSaving() const { return !bIsLoading; }
	void SetError() { bIsError = true; }
	bool IsError() const { return bIsError; }

	virtual void Serialize(void* V, int64 Length) { };
	virtual void SerializeBool(bool& D);
	virtual int64 Tell() { return -1; }
	virtual void Seek(int64 InPos) {};

	virtual int64 TotalSize() { return -1; }

	virtual FArchive& operator<<(UObject*& Value) { return *this; };
	virtual FArchive& operator<<(FName& Value) { return *this; }

	friend FArchive& operator<<(FArchive& Ar, int8& Value) { Ar.Serialize(&Value, 1); return Ar; }
	friend FArchive& operator<<(FArchive& Ar, uint8& Value) { Ar.Serialize(&Value, 1); return Ar; }
	friend FArchive& operator<<(FArchive& Ar, int16& Value) { Ar.Serialize(&Value, sizeof(Value)); return Ar; }
	friend FArchive& operator<<(FArchive& Ar, uint16& Value) { Ar.Serialize(&Value, sizeof(Value)); return Ar; }
	friend FArchive& operator<<(FArchive& Ar, int32& Value) { Ar.Serialize(&Value, sizeof(Value)); return Ar; }
	friend FArchive& operator<<(FArchive& Ar, uint32& Value) { Ar.Serialize(&Value, sizeof(Value)); return Ar; }
	friend FArchive& operator<<(FArchive& Ar, int64& Value) { Ar.Serialize(&Value, sizeof(Value)); return Ar; }
	friend FArchive& operator<<(FArchive& Ar, uint64& Value) { Ar.Serialize(&Value, sizeof(Value)); return Ar; }
	friend FArchive& operator<<(FArchive& Ar, float& Value) { Ar.Serialize(&Value, sizeof(Value)); return Ar; }
	friend FArchive& operator<<(FArchive& Ar, double& Value) { Ar.Serialize(&Value, sizeof(Value)); return Ar; }
	friend FArchive& operator<<(FArchive& Ar, bool& Value) { Ar.SerializeBool(Value); return Ar; }
	friend FArchive& operator<<(FArchive& Ar, FString& Value);

protected:
	bool bIsLoading = false;
	bool bIsError = false;
private:

};