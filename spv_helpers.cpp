#include "spv_helpers.hpp"

#include <cstring>
#include <cstdio>

static constexpr const char* invalid_string = "{Invalid}";

decoration_info get_decoration_info(Decoration d) noexcept
{
	switch (d)
	{
	case Decoration::RelaxedPrecision:
		return { "RelaxedPrecision" };
	case Decoration::SpecId:
		return { "SpecId", argument_type::literal };
	case Decoration::Block:
		return { "Block" };
	case Decoration::BufferBlock:
		return { "BufferBlock" };
	case Decoration::RowMajor:
		return { "RowMajor" };
	case Decoration::ColMajor:
		return { "ColMajor" };
	case Decoration::ArrayStride:
		return { "ArrayStride", argument_type::literal };
	case Decoration::MatrixStride:
		return { "MatrixStride", argument_type::literal };
	case Decoration::GLSLShared:
		return { "GLSLShared" };
	case Decoration::GLSLPacked:
		return { "GLSLPacked" };
	case Decoration::CPacked:
		return { "CPacked" };
	case Decoration::BuiltIn:
		return { "Builtin", argument_type::builtin };
	case Decoration::NoPerspective:
		return { "NoPerspective" };
	case Decoration::Flat:
		return { "Flat" };
	case Decoration::Patch:
		return { "Patch" };
	case Decoration::Centroid:
		return { "Centroid" };
	case Decoration::Sample:
		return { "Sample" };
	case Decoration::Invariant:
		return { "Invariant" };
	case Decoration::Restrict:
		return { "Restrict" };
	case Decoration::Aliased:
		return { "Aliased" };
	case Decoration::Volatile:
		return { "Volatile" };
	case Decoration::Constant:
		return { "Constant" };
	case Decoration::Coherent:
		return { "Coherent" };
	case Decoration::NonWritable:
		return { "NonWritable" };
	case Decoration::NonReadable:
		return { "NonReadable" };
	case Decoration::Uniform:
		return { "Uniform" };
	case Decoration::UniformId:
		return { "UniformId", argument_type::id };
	case Decoration::SaturatedConversion:
		return { "SaturatedConversion" };
	case Decoration::Stream:
		return { "Stream", argument_type::literal };
	case Decoration::Location:
		return { "Location", argument_type::literal };
	case Decoration::Component:
		return { "Component", argument_type::literal };
	case Decoration::Index:
		return { "Index", argument_type::literal };
	case Decoration::Binding:
		return { "Binding", argument_type::literal };
	case Decoration::DescriptorSet:
		return { "DescriptorSet", argument_type::literal };
	case Decoration::Offset:
		return { "Offset", argument_type::literal };
	case Decoration::XfbBuffer:
		return { "XfbBuffer", argument_type::literal };
	case Decoration::XfbStride:
		return { "XfbStride", argument_type::literal };
	case Decoration::FuncParamAttr:
		return { "FuncParamAttr", argument_type::func_param_attr };
	case Decoration::FPRoundingMode:
		return { "FPRoundingMode", argument_type::fp_rounding_mode };
	case Decoration::FPFastMathMode:
		return { "FPFastMathMode", argument_type::fp_fast_math_mode };
	case Decoration::LinkageAttributes:
		return { "LinkageAttributes", argument_type::linkage_type };
	case Decoration::NoContraction:
		return { "NoContraction" };
	case Decoration::InputAttachmentIndex:
		return { "InputAttachmentIndex", argument_type::literal };
	case Decoration::Alignment:
		return { "Alignment", argument_type::literal };
	case Decoration::MaxByteOffset:
		return { "MaxByteOffset", argument_type::literal };
	case Decoration::AlignmentId:
		return { "AlignmentId", argument_type::id };
	case Decoration::MaxByteOffsetId:
		return { "MaxByteOffsetId", argument_type::id };
	case Decoration::NoSignedWrap:
		return { "NoSignedWrap" };
	case Decoration::NoUnsignedWrap:
		return { "NoUnsignedWrap" };
	case Decoration::ExplicitInterpAMD:
		return { "ExplicitInterpAMD" };
	case Decoration::OverrideCoverageNV:
		return { "OverrideCoverageNV" };
	case Decoration::PassthroughNV:
		return { "PassthroughNV" };
	case Decoration::ViewportRelativeNV:
		return { "ViewportRelativeNV" };
	case Decoration::SecondaryViewportRelativeNV:
		return { "SecondaryViewportRelativeNV", argument_type::literal };
	case Decoration::PerPrimitiveNV:
		return { "PerPrimitiveNV" };
	case Decoration::PerViewNV:
		return { "PerViewNV" };
	case Decoration::PerTaskNV:
		return { "PerTaskNV" };
	case Decoration::PerVertexKHR:
		return { "PerVertexKHR" };
	case Decoration::NonUniform:
		return { "NonUniform" };
	case Decoration::RestrictPointer:
		return { "RestrictPointer" };
	case Decoration::AliasedPointer:
		return { "AliasedPointer" };
	case Decoration::BindlessSamplerNV:
		return { "BindlessSamplerNV" };
	case Decoration::BindlessImageNV:
		return { "BindlessImageNV" };
	case Decoration::BoundSamplerNV:
		return { "BoundSamplerNV" };
	case Decoration::BoundImageNV:
		return { "BoundImageNV" };
	case Decoration::SIMTCallINTEL:
		return { "SIMTCallINTEL", argument_type::literal };
	case Decoration::ReferencedIndirectlyINTEL:
		return { "ReferencedIndirectlyINTEL" };
	case Decoration::ClobberINTEL:
		return { "ClobberINTEL", argument_type::literal };
	case Decoration::SideEffectsINTEL:
		return { "SideEffectsINTEL" };
	case Decoration::VectorComputeVariableINTEL:
		return { "VectorComputeVariableINTEL" };
	case Decoration::FuncParamIOKindINTEL:
		return { "FuncParamIOKindINTEL", argument_type::literal };
	case Decoration::VectorComputeFunctionINTEL:
		return { "VectorComputeFunctionINTEL" };
	case Decoration::StackCallINTEL:
		return { "StackCallINTEL" };
	case Decoration::GlobalVariableOffsetINTEL:
		return { "GlobalVariableOffsetINTEL", argument_type::literal };
	case Decoration::CounterBuffer:
		return { "CounterBuffer", argument_type::id };
	case Decoration::UserSemantic:
		return { "UserSemantic", argument_type::string };
	case Decoration::UserTypeGOOGLE:
		return { "UserTypeGOOGLE", argument_type::string };
	case Decoration::FunctionRoundingModeINTEL:
		return { "FunctionRoundingModeINTEL", argument_type::literal, argument_type::fp_rounding_mode };
	case Decoration::FunctionDenormModeINTEL:
		return { "FunctionDenormModeINTEL", argument_type::literal, argument_type::reserved_fp_denorm_mode };
	case Decoration::RegisterINTEL:
		return { "RegisterINTEL" };
	case Decoration::MemoryINTEL:
		return { "MemoryINTEL", argument_type::literal };
	case Decoration::NumbanksINTEL:
		return { "NumbanksINTEL", argument_type::literal };
	case Decoration::BankwidthINTEL:
		return { "BankwidthINTEL", argument_type::literal };
	case Decoration::MaxPrivateCopiesINTEL:
		return { "MaxPrivateCopiesINTEL", argument_type::literal };
	case Decoration::SinglepumpINTEL:
		return { "SinglepumpINTEL" };
	case Decoration::DoublepumpINTEL:
		return { "DoublepumpINTEL" };
	case Decoration::MaxReplicatesINTEL:
		return { "MaxReplicatesINTEL", argument_type::literal };
	case Decoration::SimpleDualPortINTEL:
		return { "SimpleDualPortINTEL" };
	case Decoration::MergeINTEL:
		return { "MergeINTEL", argument_type::literal, argument_type::literal };
	case Decoration::BankBitsINTEL:
		return { "BankBitsINTEL", argument_type::literal };
	case Decoration::ForcePow2DepthINTEL:
		return { "ForcePow2DepthINTEL", argument_type::literal };
	case Decoration::BurstCoalesceINTEL:
		return { "BurstCoalesceINTEL" };
	case Decoration::CacheSizeINTEL:
		return { "CacheSizeINTEL", argument_type::literal };
	case Decoration::DontStaticallyCoalesceINTEL:
		return { "DontStaticallyCoalesceINTEL" };
	case Decoration::PrefetchINTEL:
		return { "PrefetchINTEL", argument_type::literal };
	case Decoration::StallEnableINTEL:
		return { "StallEnableINTEL" };
	case Decoration::FuseLoopsInFunctionINTEL:
		return { "FuseLoopsInFunctionINTEL" };
	case Decoration::BufferLocationINTEL:
		return { "BufferLocationINTEL", argument_type::literal };
	case Decoration::IOPipeStorageINTEL:
		return { "IOPipeStorageINTEL", argument_type::literal };
	case Decoration::FunctionFloatingPointModeINTEL:
		return { "FunctionFloatingPointModeINTEL", argument_type::literal, argument_type::reserved_fp_operation_mode };
	case Decoration::SingleElementVectorINTEL:
		return { "SingleElementVectorINTEL" };
	case Decoration::VectorComputeCallableFunctionINTEL:
		return { "VectorComputeCallableFunctionINTEL" };
	case Decoration::MediaBlockIOINTEL:
		return { "MediaBlockIOINTEL" };
	default:
		return { invalid_string };
	}
}

builtin_info get_builtin_info(Builtin b) noexcept
{
	static constexpr const char* names[]{
		"Position",
		"PointSize",
		invalid_string,
		"ClipDistance",
		"CullDistance",
		"VertexId",
		"InstanceId",
		"PrimitiveId",
		"InvocationId",
		"Layer",
		"ViewportIndex",
		"TessLevelOuter",
		"TessLevelInner",
		"TessCoord",
		"PatchVertices",
		"FragCoord",
		"PointCoord",
		"FrontFacing",
		"SampleId",
		"SamplePosition",
		"SampleMask",
		invalid_string,
		"FragDepth",
		"HelperInvocation",
		"NumWorkgroups",
		"WorkgroupSize",
		"WorkgroupId",
		"LocalInvocationId",
		"GlobalInvocationId",
		"LocalInvocationIndex",
		"WorkDim",
		"GlobalSize",
		"EnqueuedWorkgroupSize",
		"GlobalOffset",
		"GlobalLinearId",
		invalid_string,
		"SubgroupSize",
		"SubgroupMaxSize",
		"NumSubgroups",
		"NumEnqueuedSubgroups",
		"SubgroupId",
		"SubgroupLocalInvocationId",
		"VertexIndex",
		"InstanceIndex",
	};

	if(static_cast<uint32_t>(b) <= 43)
		return { names[static_cast<uint32_t>(b)] };

	switch (b)
	{
	case Builtin::Position:
		return { "Position" };
	case Builtin::PointSize:
		return { "PointSize" };
	case Builtin::ClipDistance:
		return { "ClipDistance" };
	case Builtin::CullDistance:
		return { "CullDistance" };
	case Builtin::VertexId:
		return { "VertexId" };
	case Builtin::InstanceId:
		return { "InstanceId" };
	case Builtin::PrimitiveId:
		return { "PrimitiveId" };
	case Builtin::InvocationId:
		return { "InvocationId" };
	case Builtin::Layer:
		return { "Layer" };
	case Builtin::ViewportIndex:
		return { "ViewportIndex" };
	case Builtin::TessLevelOuter:
		return { "TessLevelOuter" };
	case Builtin::TessLevelInner:
		return { "TessLevelInner" };
	case Builtin::TessCoord:
		return { "TessCoord" };
	case Builtin::PatchVertices:
		return { "PatchVertices" };
	case Builtin::FragCoord:
		return { "FragCoord" };
	case Builtin::PointCoord:
		return { "PointCoord" };
	case Builtin::FrontFacing:
		return { "FrontFacing" };
	case Builtin::SampleId:
		return { "SampleId" };
	case Builtin::SamplePosition:
		return { "SamplePosition" };
	case Builtin::SampleMask:
		return { "SampleMask" };
	case Builtin::FragDepth:
		return { "FragDepth" };
	case Builtin::HelperInvocation:
		return { "HelperInvocation" };
	case Builtin::NumWorkgroups:
		return { "NumWorkgroups" };
	case Builtin::WorkgroupSize:
		return { "WorkgroupSize" };
	case Builtin::WorkgroupId:
		return { "WorkgroupId" };
	case Builtin::LocalInvocationId:
		return { "LocalInvocationId" };
	case Builtin::GlobalInvocationId:
		return { "GlobalInvocationId" };
	case Builtin::LocalInvocationIndex:
		return { "LocalInvocationIndex" };
	case Builtin::WorkDim:
		return { "WorkDim" };
	case Builtin::GlobalSize:
		return { "GlobalSize" };
	case Builtin::EnqueuedWorkgroupSize:
		return { "EnqueuedWorkgroupSize" };
	case Builtin::GlobalOffset:
		return { "GlobalOffset" };
	case Builtin::GlobalLinearId:
		return { "GlobalLinearId" };
	case Builtin::SubgroupSize:
		return { "SubgroupSize" };
	case Builtin::SubgroupMaxSize:
		return { "SubgroupMaxSize" };
	case Builtin::NumSubgroups:
		return { "NumSubgroups" };
	case Builtin::NumEnqueuedSubgroups:
		return { "NumEnqueuedSubgroups" };
	case Builtin::SubgroupId:
		return { "SubgroupId" };
	case Builtin::SubgroupLocalInvocationId:
		return { "SubgroupLocalInvocationId" };
	case Builtin::VertexIndex:
		return { "VertexIndex" };
	case Builtin::InstanceIndex:
		return { "InstanceIndex" };
	case Builtin::SubgroupEqMask:
		return { "SubgroupEqMask" };
	case Builtin::SubgroupGeMask:
		return { "SubgroupGeMask" };
	case Builtin::SubgroupGtMask:
		return { "SubgroupGtMask" };
	case Builtin::SubgroupLeMask:
		return { "SubgroupLeMask" };
	case Builtin::SubgroupLtMask:
		return { "SubgroupLtMask" };
	case Builtin::BaseVertex:
		return { "BaseVertex" };
	case Builtin::BaseInstance:
		return { "BaseInstance" };
	case Builtin::DrawIndex:
		return { "DrawIndex" };
	case Builtin::PrimitiveShadingRateKHR:
		return { "PrimitiveShadingRateKHR" };
	case Builtin::DeviceIndex:
		return { "DeviceIndex" };
	case Builtin::ViewIndex:
		return { "ViewIndex" };
	case Builtin::ShadingRateKHR:
		return { "ShadingRateKHR" };
	case Builtin::BaryCoordNoPerspAMD:
		return { "BaryCoordNoPerspAMD" };
	case Builtin::BaryCoordNoPerspCentroidAMD:
		return { "BaryCoordNoPerspCentroidAMD" };
	case Builtin::BaryCoordNoPerspSampleAMD:
		return { "BaryCoordNoPerspSampleAMD" };
	case Builtin::BaryCoordSmoothAMD:
		return { "BaryCoordSmoothAMD" };
	case Builtin::BaryCoordSmoothCentroidAMD:
		return { "BaryCoordSmoothCentroidAMD" };
	case Builtin::BaryCoordSmoothSampleAMD:
		return { "BaryCoordSmoothSampleAMD" };
	case Builtin::BaryCoordPullModelAMD:
		return { "BaryCoordPullModelAMD" };
	case Builtin::FragStencilRefEXT:
		return { "FragStencilRefEXT" };
	case Builtin::ViewportMaskNV:
		return { "ViewportMaskNV" };
	case Builtin::SecondaryPositionNV:
		return { "SecondaryPositionNV" };
	case Builtin::SecondaryViewportMaskNV:
		return { "SecondaryViewportMaskNV" };
	case Builtin::PositionPerViewNV:
		return { "PositionPerViewNV" };
	case Builtin::ViewportMaskPerViewNV:
		return { "ViewportMaskPerViewNV" };
	case Builtin::FullyCoveredEXT:
		return { "FullyCoveredEXT" };
	case Builtin::TaskCountNV:
		return { "TaskCountNV" };
	case Builtin::PrimitiveCountNV:
		return { "PrimitiveCountNV" };
	case Builtin::PrimitiveIndicesNV:
		return { "PrimitiveIndicesNV" };
	case Builtin::ClipDistancePerViewNV:
		return { "ClipDistancePerViewNV" };
	case Builtin::CullDistancePerViewNV:
		return { "CullDistancePerViewNV" };
	case Builtin::LayerPerViewNV:
		return { "LayerPerViewNV" };
	case Builtin::MeshViewCountNV:
		return { "MeshViewCountNV" };
	case Builtin::MeshViewIndicesNV:
		return { "MeshViewIndicesNV" };
	case Builtin::BaryCoordKHR:
		return { "BaryCoordKHR" };
	case Builtin::BaryCoordNoPerspKHR:
		return { "BaryCoordNoPerspKHR" };
	case Builtin::FragSizeEXT:
		return { "FragSizeEXT" };
	case Builtin::FragInvocationCountEXT:
		return { "FragInvocationCountEXT" };
	case Builtin::LaunchIdKHR:
		return { "LaunchIdKHR" };
	case Builtin::LaunchSizeKHR:
		return { "LaunchSizeKHR" };
	case Builtin::WorldRayOriginKHR:
		return { "WorldRayOriginKHR" };
	case Builtin::WorldRayDirectionKHR:
		return { "WorldRayDirectionKHR" };
	case Builtin::ObjectRayOriginKHR:
		return { "ObjectRayOriginKHR" };
	case Builtin::ObjectRayDirectionKHR:
		return { "ObjectRayDirectionKHR" };
	case Builtin::RayTminKHR:
		return { "RayTminKHR" };
	case Builtin::RayTmaxKHR:
		return { "RayTmaxKHR" };
	case Builtin::InstanceCustomIndexKHR:
		return { "InstanceCustomIndexKHR" };
	case Builtin::ObjectToWorldKHR:
		return { "ObjectToWorldKHR" };
	case Builtin::WorldToObjectKHR:
		return { "WorldToObjectKHR" };
	case Builtin::HitTNV:
		return { "HitTNV" };
	case Builtin::HitKindKHR:
		return { "HitKindKHR" };
	case Builtin::CurrentRayTimeNV:
		return { "CurrentRayTimeNV" };
	case Builtin::IncomingRayFlagsKHR:
		return { "IncomingRayFlagsKHR" };
	case Builtin::RayGeometryIndexKHR:
		return { "RayGeometryIndexKHR" };
	case Builtin::WarpsPerSMNV:
		return { "WarpsPerSMNV" };
	case Builtin::SMCountNV:
		return { "SMCountNV" };
	case Builtin::WarpIDNV:
		return { "WarpIDNV" };
	case Builtin::SMIDNV:
		return { "SMIDNV" };
	default:
		return { invalid_string };
	}
}

function_param_attribute_info get_function_param_attribute_info(FunctionParamAttribute a) noexcept
{
	static constexpr const char* names[]{
		"Zext",
		"Sext",
		"ByVal",
		"Sret",
		"NoAlias",
		"NoCapture",
		"NoWrite",
		"NoReadWrite",
	};

	if(static_cast<uint32_t>(a) > 7)
		return { invalid_string };

	return { names[static_cast<uint32_t>(a)] };
}

fp_rounding_mode_info get_fp_rounding_mode_info(FpRoundingMode m) noexcept
{
	static constexpr const char names[4][4]{
		"RTE",
		"RTZ",
		"RTP",
		"RTN",
	};

	if (static_cast<uint32_t>(m) > 3)
		return { invalid_string };

	return { names[static_cast<uint32_t>(m)] };
}

fp_fast_math_mode_info get_fp_fast_math_mode_info(FpFastMathMode m) noexcept
{
	switch(m)
	{
		case FpFastMathMode::None:
			return { "None" };
		case FpFastMathMode::NotNaN:
			return { "NotNaN" };
		case FpFastMathMode::NotInf:
			return { "NotInf" };
		case FpFastMathMode::NSZ:
			return { "NSZ" };
		case FpFastMathMode::AllowRecip:
			return { "AllowRecip" };
		case FpFastMathMode::Fast:
			return { "Fast" };
		case FpFastMathMode::AllowContractFastINTEL:
			return { "AllowContractFastINTEL" };
		case FpFastMathMode::AllowContractReassocINTEL:
			return { "AllowContractReassocINTEL" };
		default:
			return { invalid_string };
	}
}

linkage_type_info get_linkage_type_info(LinkageType t) noexcept
{
	static constexpr const char* names[]{
		"Export",
		"Import",
		"LinkOnceODR",
	};

	if(static_cast<uint32_t>(t) > 2)
		return { invalid_string };

	return { names[static_cast<uint32_t>(t)] };
}

reserved_fp_denorm_mode_info get_reserved_fp_denorm_mode_info(ReservedFpDenormMode m) noexcept
{
	if(static_cast<uint32_t>(m) == 0)
		return { "Preserve" };
	else if(static_cast<uint32_t>(m) == 1)
		return { "FlushToZero" };
	else
		return { invalid_string };
}

reserved_fp_operation_mode_info get_reserved_fp_operation_mode_info(ReservedFpOperationMode m) noexcept
{
	if(static_cast<uint32_t>(m) == 0)
		return { "IEEE" };
	else if(static_cast<uint32_t>(m) == 1)
		return { "ALT" };
	else
		return { invalid_string };
}

source_language_info get_source_language_info(SourceLanguage l) noexcept
{
	static constexpr const char* names[]{
		"Unknown",
		"ESSL",
		"GLSL",
		"OpenCL_C",
		"OpenCL_CPP",
		"HLSL",
		"CPP_for_OpenCL",
	};

	if(static_cast<uint32_t>(l) > 6)
		return { invalid_string };

	return { names[static_cast<uint32_t>(l)] };
}
