#pragma once

#include "CoreMinimal.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "SceneTexturesConfig.h"
#include "ShaderCore.h"
#include "ShaderParameterStruct.h"

namespace AxisChartCompute
{
	static constexpr int32 THREADS_X = 16;
	static constexpr int32 THREADS_Y = 16;
	static constexpr int32 THREADS_Z = 1;
}

BEGIN_SHADER_PARAMETER_STRUCT(FAxisChartGenCSParams,)
	SHADER_PARAMETER(uint32, RowCount)
	SHADER_PARAMETER(uint32, ColCount)
	SHADER_PARAMETER(float, WidthX)
	SHADER_PARAMETER(float, WidthY)
	SHADER_PARAMETER(float, SpaceX)
	SHADER_PARAMETER(float, SpaceY)
	SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float>, Height)
END_SHADER_PARAMETER_STRUCT()

/**
 * 考虑所有使用坐标轴的图表
 */
class FAxisChartGenCS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FAxisChartGenCS, Global,);
	using FParameters = FAxisChartGenCSParams;
	SHADER_USE_PARAMETER_STRUCT(FAxisChartGenCS, FGlobalShader);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
	
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);

		// When changing these, you may need to change something in the shader for it to take effect
		// A simple comment with a bit of gibberish seems to be enough
		OutEnvironment.SetDefine(TEXT("THREADS_X"), AxisChartCompute::THREADS_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), AxisChartCompute::THREADS_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), AxisChartCompute::THREADS_Z);
	}
};