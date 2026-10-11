#include "ResortAudio.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"
#include "Sound/SoundWaveProcedural.h"
#include "UObject/Package.h"

namespace
{
	constexpr int32 SampleRate = 22050;

	TArray<int16> Make(float Seconds)
	{
		TArray<int16> Out;
		Out.SetNumZeroed(FMath::Max(1, FMath::RoundToInt(Seconds * SampleRate)));
		return Out;
	}

	int16 ToPcm(float V)
	{
		return static_cast<int16>(FMath::Clamp(V, -1.f, 1.f) * 30000.f);
	}

	TArray<int16> MakeTone(const TArray<float>& Freqs, float Seconds, float Decay, float Gain)
	{
		TArray<int16> Out = Make(Seconds);
		for (int32 i = 0; i < Out.Num(); ++i)
		{
			const float T = static_cast<float>(i) / SampleRate;
			float V = 0.f;
			for (float F : Freqs)
			{
				V += FMath::Sin(2.f * PI * F * T);
			}
			V /= FMath::Max(1, Freqs.Num());
			Out[i] = ToPcm(V * FMath::Exp(-T * Decay) * Gain);
		}
		return Out;
	}

	TArray<int16> MakeCoin()
	{
		// duas notas altas, tipo "ding" de caixa
		TArray<int16> A = MakeTone({ 1319.f, 2637.f }, 0.4f, 9.f, 0.5f);
		TArray<int16> B = MakeTone({ 1760.f, 3520.f }, 0.4f, 9.f, 0.5f);
		const int32 Offset = SampleRate / 12;
		TArray<int16> Out = Make(0.4f + 1.f / 12.f);
		for (int32 i = 0; i < A.Num(); ++i)
		{
			Out[i] = A[i];
		}
		for (int32 i = 0; i < B.Num() && i + Offset < Out.Num(); ++i)
		{
			Out[i + Offset] = static_cast<int16>(FMath::Clamp<int32>(Out[i + Offset] + B[i], -32000, 32000));
		}
		return Out;
	}

	TArray<int16> MakeBuild()
	{
		// batida grave com um pouco de ruido
		FRandomStream R(7);
		TArray<int16> Out = Make(0.3f);
		for (int32 i = 0; i < Out.Num(); ++i)
		{
			const float T = static_cast<float>(i) / SampleRate;
			const float Thump = FMath::Sin(2.f * PI * (110.f - 60.f * T) * T) * FMath::Exp(-T * 14.f);
			const float Noise = (R.FRand() * 2.f - 1.f) * FMath::Exp(-T * 40.f) * 0.4f;
			Out[i] = ToPcm((Thump + Noise) * 0.8f);
		}
		return Out;
	}

	TArray<int16> MakeUpgrade()
	{
		// tres notas subindo
		const float Notes[3] = { 784.f, 988.f, 1319.f };
		TArray<int16> Out = Make(0.75f);
		for (int32 n = 0; n < 3; ++n)
		{
			const int32 Start = n * SampleRate / 8;
			for (int32 i = 0; Start + i < Out.Num(); ++i)
			{
				const float T = static_cast<float>(i) / SampleRate;
				const float V = FMath::Sin(2.f * PI * Notes[n] * T) * FMath::Exp(-T * 7.f) * 0.45f;
				Out[Start + i] = static_cast<int16>(FMath::Clamp<int32>(Out[Start + i] + ToPcm(V), -32000, 32000));
			}
		}
		return Out;
	}

	TArray<int16> MakeGull()
	{
		// dois pios com frequencia que cai
		TArray<int16> Out = Make(1.1f);
		const float Starts[2] = { 0.f, 0.45f };
		for (int32 c = 0; c < 2; ++c)
		{
			const int32 Start = FMath::RoundToInt(Starts[c] * SampleRate);
			const int32 Len = SampleRate * 4 / 10;
			float Phase = 0.f;
			for (int32 i = 0; i < Len && Start + i < Out.Num(); ++i)
			{
				const float T = static_cast<float>(i) / Len;
				const float Freq = 2200.f - 900.f * T + 120.f * FMath::Sin(T * 40.f);
				Phase += 2.f * PI * Freq / SampleRate;
				const float Env = FMath::Sin(T * PI);
				Out[Start + i] = static_cast<int16>(FMath::Clamp<int32>(Out[Start + i] + ToPcm(FMath::Sin(Phase) * Env * 0.3f), -32000, 32000));
			}
		}
		return Out;
	}

	TArray<int16> MakeError()
	{
		return MakeTone({ 160.f, 170.f }, 0.25f, 8.f, 0.6f);
	}

	TArray<int16> MakeSea()
	{
		// Ruido suave com ondas lentas; as pontas se misturam para o som poder repetir sem corte.
		const float Seconds = 12.f;
		const int32 N = FMath::RoundToInt(Seconds * SampleRate);
		const int32 Fade = SampleRate;
		FRandomStream R(42);
		TArray<float> X;
		X.SetNumZeroed(N + Fade);
		float Low = 0.f;
		for (int32 i = 0; i < X.Num(); ++i)
		{
			const float Noise = R.FRand() * 2.f - 1.f;
			Low += 0.06f * (Noise - Low);
			const float T = static_cast<float>(i) / SampleRate;
			const float Swell = 0.55f + 0.45f * FMath::Sin(2.f * PI * T / 6.f);
			X[i] = Low * 3.2f * Swell;
		}
		TArray<int16> Out = Make(Seconds);
		for (int32 i = 0; i < N; ++i)
		{
			float V = X[i];
			if (i < Fade)
			{
				const float A = static_cast<float>(i) / Fade;
				V = X[i] * A + X[N + i] * (1.f - A);
			}
			Out[i] = ToPcm(V * 0.5f);
		}
		return Out;
	}

	const TArray<int16>& GetSea()
	{
		static TArray<int16> Sea = MakeSea();
		return Sea;
	}

	const TArray<int16>& GetPcm(EResortSound Sound)
	{
		static TArray<int16> Coin = MakeCoin();
		static TArray<int16> Build = MakeBuild();
		static TArray<int16> Upgrade = MakeUpgrade();
		static TArray<int16> Gull = MakeGull();
		static TArray<int16> Error = MakeError();
		switch (Sound)
		{
		case EResortSound::Coin: return Coin;
		case EResortSound::Build: return Build;
		case EResortSound::Upgrade: return Upgrade;
		case EResortSound::Gull: return Gull;
		default: return Error;
		}
	}

	USoundWaveProcedural* NewWave(float DurationSeconds, bool bIndefinite)
	{
		USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(GetTransientPackage());
		Wave->SetSampleRate(SampleRate);
		Wave->NumChannels = 1;
		Wave->Duration = bIndefinite ? INDEFINITELY_LOOPING_DURATION : DurationSeconds;
		Wave->SoundGroup = SOUNDGROUP_Default;
		Wave->bLooping = false;
		return Wave;
	}

	void Queue(USoundWaveProcedural* Wave, const TArray<int16>& Pcm)
	{
		Wave->QueueAudio(reinterpret_cast<const uint8*>(Pcm.GetData()), Pcm.Num() * sizeof(int16));
	}
}

void ResortAudio::Play(const UObject* WorldContext, EResortSound Sound, float Volume)
{
	if (WorldContext == nullptr || WorldContext->GetWorld() == nullptr)
	{
		return;
	}
	const TArray<int16>& Pcm = GetPcm(Sound);
	USoundWaveProcedural* Wave = NewWave(static_cast<float>(Pcm.Num()) / SampleRate, false);
	Queue(Wave, Pcm);
	UGameplayStatics::PlaySound2D(WorldContext, Wave, Volume);
}

USoundWaveProcedural* ResortAudio::CreateSeaAmbience()
{
	USoundWaveProcedural* Wave = NewWave(0.f, true);
	Queue(Wave, GetSea());
	return Wave;
}

void ResortAudio::KeepSeaAmbienceFed(USoundWaveProcedural* Wave)
{
	if (Wave != nullptr && Wave->GetAvailableAudioByteCount() < static_cast<int32>(SampleRate * sizeof(int16) * 4))
	{
		Queue(Wave, GetSea());
	}
}
