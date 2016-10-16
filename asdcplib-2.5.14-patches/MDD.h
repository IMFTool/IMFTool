/*
Copyright (c) 2006-2016, John Hurst
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*! \file    MDD.[h|cpp]
    \version $Id: MDD.h,v 1.37 2016/05/09 18:18:42 jhurst Exp $
    \brief   MXF Metadata Dictionary
*/

#ifndef _MDD_H_
#define _MDD_H_

//
namespace ASDCP {
    enum MDD_t {
        MDD_MICAlgorithm_NONE,  // 0
        MDD_MXFInterop_OPAtom,  // 1
        MDD_OPAtom,  // 2
        MDD_OP1a,  // 3
        MDD_GCMulti,  // 4
        MDD_PictureDataDef,  // 5
        MDD_SoundDataDef,  // 6
        MDD_TimecodeDataDef,  // 7
        MDD_DescriptiveMetaDataDef,  // 8
        MDD_WAVWrappingFrame,  // 9
        MDD_MPEG2_VESWrappingFrame,  // 10
        MDD_JPEG_2000WrappingFrame,  // 11
        MDD_JPEG2000Essence,  // 12
        MDD_MPEG2Essence,  // 13
        MDD_MXFInterop_CryptEssence,  // 14
        MDD_CryptEssence,  // 15
        MDD_WAVEssence,  // 16
        MDD_JP2KEssenceCompression_2K,  // 17
        MDD_JP2KEssenceCompression_4K,  // 18
        MDD_CipherAlgorithm_AES,  // 19
        MDD_MICAlgorithm_HMAC_SHA1,  // 20
        MDD_KLVFill,  // 21
        MDD_PartitionMetadata_MajorVersion,  // 22
        MDD_PartitionMetadata_MinorVersion,  // 23
        MDD_PartitionMetadata_KAGSize,  // 24
        MDD_PartitionMetadata_ThisPartition,  // 25
        MDD_PartitionMetadata_PreviousPartition,  // 26
        MDD_PartitionMetadata_FooterPartition,  // 27
        MDD_PartitionMetadata_HeaderByteCount,  // 28
        MDD_PartitionMetadata_IndexByteCount,  // 29
        MDD_PartitionMetadata_IndexSID_DEPRECATED,  // 30
        MDD_PartitionMetadata_BodyOffset,  // 31
        MDD_PartitionMetadata_BodySID_DEPRECATED,  // 32
        MDD_PartitionMetadata_OperationalPattern_DEPRECATED,  // 33
        MDD_PartitionMetadata_EssenceContainers_DEPRECATED,  // 34
        MDD_OpenHeader,  // 35
        MDD_OpenCompleteHeader,  // 36
        MDD_ClosedHeader,  // 37
        MDD_ClosedCompleteHeader,  // 38
        MDD_OpenBodyPartition,  // 39
        MDD_OpenCompleteBodyPartition,  // 40
        MDD_ClosedBodyPartition,  // 41
        MDD_ClosedCompleteBodyPartition,  // 42
        MDD_Footer,  // 43
        MDD_CompleteFooter,  // 44
        MDD_Primer,  // 45
        MDD_Primer_LocalTagEntryBatch,  // 46
        MDD_LocalTagEntryBatch_Primer_LocalTag,  // 47
        MDD_LocalTagEntryBatch_Primer_UID,  // 48
        MDD_InterchangeObject_InstanceUID,  // 49
        MDD_GenerationInterchangeObject_GenerationUID,  // 50
        MDD_DefaultObject,  // 51
        MDD_IndexTableSegmentBase_IndexEditRate,  // 52
        MDD_IndexTableSegmentBase_IndexStartPosition,  // 53
        MDD_IndexTableSegmentBase_IndexDuration,  // 54
        MDD_IndexTableSegmentBase_EditUnitByteCount,  // 55
        MDD_IndexTableSegmentBase_IndexSID_DEPRECATED,  // 56
        MDD_IndexTableSegmentBase_BodySID_DEPRECATED,  // 57
        MDD_IndexTableSegmentBase_SliceCount,  // 58
        MDD_IndexTableSegmentBase_PosTableCount,  // 59
        MDD_IndexTableSegment,  // 60
        MDD_IndexTableSegment_DeltaEntryArray,  // 61
        MDD_DeltaEntryArray_IndexTableSegment_PosTableIndex,  // 62
        MDD_DeltaEntryArray_IndexTableSegment_Slice,  // 63
        MDD_DeltaEntryArray_IndexTableSegment_ElementDelta,  // 64
        MDD_IndexTableSegment_IndexEntryArray,  // 65
        MDD_IndexEntryArray_IndexTableSegment_TemporalOffset,  // 66
        MDD_IndexEntryArray_IndexTableSegment_AnchorOffset,  // 67
        MDD_IndexEntryArray_IndexTableSegment_Flags,  // 68
        MDD_IndexEntryArray_IndexTableSegment_StreamOffset,  // 69
        MDD_IndexEntryArray_IndexTableSegment_SliceOffsetArray,  // 70
        MDD_IndexEntryArray_IndexTableSegment_PosTableArray,  // 71
        MDD_RandomIndexMetadata,  // 72
        MDD_PartitionArray_RandomIndexMetadata_BodySID_DEPRECATED,  // 73
        MDD_PartitionArray_RandomIndexMetadata_ByteOffset,  // 74
        MDD_RandomIndexMetadata_Length,  // 75
        MDD_RandomIndexMetadataV10,  // 76
        MDD_Preface,  // 77
        MDD_Preface_LastModifiedDate,  // 78
        MDD_Preface_Version,  // 79
        MDD_Preface_ObjectModelVersion,  // 80
        MDD_Preface_PrimaryPackage,  // 81
        MDD_Preface_Identifications,  // 82
        MDD_Preface_ContentStorage,  // 83
        MDD_Preface_OperationalPattern_DEPRECATED,  // 84
        MDD_Preface_EssenceContainers_DEPRECATED,  // 85
        MDD_Preface_DMSchemes,  // 86
        MDD_Identification,  // 87
        MDD_Identification_ThisGenerationUID,  // 88
        MDD_Identification_CompanyName,  // 89
        MDD_Identification_ProductName,  // 90
        MDD_Identification_ProductVersion,  // 91
        MDD_Identification_VersionString,  // 92
        MDD_Identification_ProductUID,  // 93
        MDD_Identification_ModificationDate,  // 94
        MDD_Identification_ToolkitVersion,  // 95
        MDD_Identification_Platform,  // 96
        MDD_ContentStorage,  // 97
        MDD_ContentStorage_Packages,  // 98
        MDD_ContentStorage_EssenceContainerData,  // 99
        MDD_ContentStorageKludge_V10Packages,  // 100
        MDD_EssenceContainerData,  // 101
        MDD_EssenceContainerData_LinkedPackageUID,  // 102
        MDD_EssenceContainerData_IndexSID_DEPRECATED,  // 103
        MDD_EssenceContainerData_BodySID_DEPRECATED,  // 104
        MDD_GenericPackage_PackageUID,  // 105
        MDD_GenericPackage_Name,  // 106
        MDD_GenericPackage_PackageCreationDate,  // 107
        MDD_GenericPackage_PackageModifiedDate,  // 108
        MDD_GenericPackage_Tracks,  // 109
        MDD_NetworkLocator,  // 110
        MDD_NetworkLocator_URLString,  // 111
        MDD_TextLocator,  // 112
        MDD_TextLocator_LocatorName,  // 113
        MDD_GenericTrack_TrackID,  // 114
        MDD_GenericTrack_TrackNumber,  // 115
        MDD_GenericTrack_TrackName,  // 116
        MDD_GenericTrack_Sequence,  // 117
        MDD_StaticTrack,  // 118
        MDD_Track,  // 119
        MDD_Track_EditRate,  // 120
        MDD_Track_Origin,  // 121
        MDD_EventTrack,  // 122
        MDD_EventTrack_EventEditRate,  // 123
        MDD_EventTrack_EventOrigin,  // 124
        MDD_StructuralComponent_DataDefinition,  // 125
        MDD_StructuralComponent_Duration,  // 126
        MDD_Sequence,  // 127
        MDD_Sequence_StructuralComponents,  // 128
        MDD_TimecodeComponent,  // 129
        MDD_TimecodeComponent_RoundedTimecodeBase,  // 130
        MDD_TimecodeComponent_StartTimecode,  // 131
        MDD_TimecodeComponent_DropFrame,  // 132
        MDD_SourceClip,  // 133
        MDD_SourceClip_StartPosition,  // 134
        MDD_SourceClip_SourcePackageID,  // 135
        MDD_SourceClip_SourceTrackID,  // 136
        MDD_DMSegment,  // 137
        MDD_DMSegment_EventStartPosition,  // 138
        MDD_DMSegment_EventComment,  // 139
        MDD_DMSegment_TrackIDs,  // 140
        MDD_DMSegment_DMFramework,  // 141
        MDD_DMSourceClip,  // 142
        MDD_DMSourceClip_DMSourceClipTrackIDs,  // 143
        MDD_MaterialPackage,  // 144
        MDD_SourcePackage,  // 145
        MDD_SourcePackage_Descriptor,  // 146
        MDD_GenericDescriptor_Locators,  // 147
        MDD_GenericDescriptor_SubDescriptors,  // 148
        MDD_FileDescriptor,  // 149
        MDD_FileDescriptor_LinkedTrackID,  // 150
        MDD_FileDescriptor_SampleRate,  // 151
        MDD_FileDescriptor_ContainerDuration,  // 152
        MDD_FileDescriptor_EssenceContainer,  // 153
        MDD_FileDescriptor_Codec,  // 154
        MDD_GenericPictureEssenceDescriptor,  // 155
        MDD_GenericPictureEssenceDescriptor_SignalStandard,  // 156
        MDD_GenericPictureEssenceDescriptor_FrameLayout,  // 157
        MDD_GenericPictureEssenceDescriptor_StoredWidth,  // 158
        MDD_GenericPictureEssenceDescriptor_StoredHeight,  // 159
        MDD_GenericPictureEssenceDescriptor_StoredF2Offset,  // 160
        MDD_GenericPictureEssenceDescriptor_SampledWidth,  // 161
        MDD_GenericPictureEssenceDescriptor_SampledHeight,  // 162
        MDD_GenericPictureEssenceDescriptor_SampledXOffset,  // 163
        MDD_GenericPictureEssenceDescriptor_SampledYOffset,  // 164
        MDD_GenericPictureEssenceDescriptor_DisplayHeight,  // 165
        MDD_GenericPictureEssenceDescriptor_DisplayWidth,  // 166
        MDD_GenericPictureEssenceDescriptor_DisplayXOffset,  // 167
        MDD_GenericPictureEssenceDescriptor_DisplayYOffset,  // 168
        MDD_GenericPictureEssenceDescriptor_DisplayF2Offset,  // 169
        MDD_GenericPictureEssenceDescriptor_AspectRatio,  // 170
        MDD_GenericPictureEssenceDescriptor_ActiveFormatDescriptor,  // 171
        MDD_GenericPictureEssenceDescriptor_VideoLineMap,  // 172
        MDD_GenericPictureEssenceDescriptor_AlphaTransparency,  // 173
        MDD_GenericPictureEssenceDescriptor_TransferCharacteristic,  // 174
        MDD_GenericPictureEssenceDescriptor_ImageAlignmentOffset,  // 175
        MDD_GenericPictureEssenceDescriptor_ImageStartOffset,  // 176
        MDD_GenericPictureEssenceDescriptor_ImageEndOffset,  // 177
        MDD_GenericPictureEssenceDescriptor_FieldDominance,  // 178
        MDD_GenericPictureEssenceDescriptor_PictureEssenceCoding,  // 179
        MDD_CDCIEssenceDescriptor,  // 180
        MDD_CDCIEssenceDescriptor_ComponentDepth,  // 181
        MDD_CDCIEssenceDescriptor_HorizontalSubsampling,  // 182
        MDD_CDCIEssenceDescriptor_VerticalSubsampling,  // 183
        MDD_CDCIEssenceDescriptor_ColorSiting,  // 184
        MDD_CDCIEssenceDescriptor_ReversedByteOrder,  // 185
        MDD_CDCIEssenceDescriptor_PaddingBits,  // 186
        MDD_CDCIEssenceDescriptor_AlphaSampleDepth,  // 187
        MDD_CDCIEssenceDescriptor_BlackRefLevel,  // 188
        MDD_CDCIEssenceDescriptor_WhiteReflevel,  // 189
        MDD_CDCIEssenceDescriptor_ColorRange,  // 190
        MDD_RGBAEssenceDescriptor,  // 191
        MDD_RGBAEssenceDescriptor_ComponentMaxRef,  // 192
        MDD_RGBAEssenceDescriptor_ComponentMinRef,  // 193
        MDD_RGBAEssenceDescriptor_AlphaMaxRef,  // 194
        MDD_RGBAEssenceDescriptor_AlphaMinRef,  // 195
        MDD_RGBAEssenceDescriptor_ScanningDirection,  // 196
        MDD_RGBAEssenceDescriptor_PixelLayout,  // 197
        MDD_RGBAEssenceDescriptor_Palette,  // 198
        MDD_RGBAEssenceDescriptor_PaletteLayout,  // 199
        MDD_GenericSoundEssenceDescriptor,  // 200
        MDD_GenericSoundEssenceDescriptor_AudioSamplingRate,  // 201
        MDD_GenericSoundEssenceDescriptor_Locked,  // 202
        MDD_GenericSoundEssenceDescriptor_AudioRefLevel,  // 203
        MDD_GenericSoundEssenceDescriptor_ElectroSpatialFormulation,  // 204
        MDD_GenericSoundEssenceDescriptor_ChannelCount,  // 205
        MDD_GenericSoundEssenceDescriptor_QuantizationBits,  // 206
        MDD_GenericSoundEssenceDescriptor_DialNorm,  // 207
        MDD_GenericSoundEssenceDescriptor_SoundEssenceCoding,  // 208
        MDD_GenericDataEssenceDescriptor,  // 209
        MDD_GenericDataEssenceDescriptor_DataEssenceCoding,  // 210
        MDD_MultipleDescriptor,  // 211
        MDD_MultipleDescriptor_SubDescriptorUIDs,  // 212
        MDD_MPEG2VideoDescriptor,  // 213
        MDD_MPEG2VideoDescriptor_SingleSequence,  // 214
        MDD_MPEG2VideoDescriptor_ConstantBFrames,  // 215
        MDD_MPEG2VideoDescriptor_CodedContentType,  // 216
        MDD_MPEG2VideoDescriptor_LowDelay,  // 217
        MDD_MPEG2VideoDescriptor_ClosedGOP,  // 218
        MDD_MPEG2VideoDescriptor_IdenticalGOP,  // 219
        MDD_MPEG2VideoDescriptor_MaxGOP,  // 220
        MDD_MPEG2VideoDescriptor_BPictureCount,  // 221
        MDD_MPEG2VideoDescriptor_BitRate,  // 222
        MDD_MPEG2VideoDescriptor_ProfileAndLevel,  // 223
        MDD_WaveAudioDescriptor,  // 224
        MDD_WaveAudioDescriptor_BlockAlign,  // 225
        MDD_WaveAudioDescriptor_SequenceOffset,  // 226
        MDD_WaveAudioDescriptor_AvgBps,  // 227
        MDD_WaveAudioDescriptor_PeakEnvelope,  // 228
        MDD_JPEG2000PictureSubDescriptor,  // 229
        MDD_JPEG2000PictureSubDescriptor_Rsize,  // 230
        MDD_JPEG2000PictureSubDescriptor_Xsize,  // 231
        MDD_JPEG2000PictureSubDescriptor_Ysize,  // 232
        MDD_JPEG2000PictureSubDescriptor_XOsize,  // 233
        MDD_JPEG2000PictureSubDescriptor_YOsize,  // 234
        MDD_JPEG2000PictureSubDescriptor_XTsize,  // 235
        MDD_JPEG2000PictureSubDescriptor_YTsize,  // 236
        MDD_JPEG2000PictureSubDescriptor_XTOsize,  // 237
        MDD_JPEG2000PictureSubDescriptor_YTOsize,  // 238
        MDD_JPEG2000PictureSubDescriptor_Csize,  // 239
        MDD_JPEG2000PictureSubDescriptor_PictureComponentSizing,  // 240
        MDD_JPEG2000PictureSubDescriptor_CodingStyleDefault,  // 241
        MDD_JPEG2000PictureSubDescriptor_QuantizationDefault,  // 242
        MDD_DM_Framework,  // 243
        MDD_DM_Set,  // 244
        MDD_EncryptedContainerLabel,  // 245
        MDD_CryptographicFrameworkLabel,  // 246
        MDD_CryptographicFramework,  // 247
        MDD_CryptographicFramework_ContextSR,  // 248
        MDD_CryptographicContext,  // 249
        MDD_CryptographicContext_ContextID,  // 250
        MDD_CryptographicContext_SourceEssenceContainer,  // 251
        MDD_CryptographicContext_CipherAlgorithm,  // 252
        MDD_CryptographicContext_MICAlgorithm,  // 253
        MDD_CryptographicContext_CryptographicKeyID,  // 254
        MDD_TimedTextWrappingClip, // 255
        MDD_TimedTextEssence, // 256
        MDD_TimedTextDescriptor, // 257
        MDD_TimedTextDescriptor_ResourceID, // 258
        MDD_TimedTextDescriptor_UCSEncoding, // 259
        MDD_TimedTextDescriptor_NamespaceURI, // 260
        MDD_TimedTextResourceSubDescriptor, // 261
        MDD_TimedTextResourceSubDescriptor_AncillaryResourceID, // 262
        MDD_TimedTextResourceSubDescriptor_MIMEMediaType, // 263
        MDD_TimedTextResourceSubDescriptor_EssenceStreamID_DEPRECATED, // 264
        MDD_GenericStreamPartition, // 265
        MDD_DMSegment_DataDefinition_DEPRECATED, // 266
        MDD_DMSegment_Duration_DEPRECATED, // 267
        MDD_DMSegment_TrackIDList, // 268
        MDD_StereoscopicPictureSubDescriptor, // 269
        MDD_WaveAudioDescriptor_ChannelAssignment,  // 270
        MDD_GenericStream_DataElement, // 271
        MDD_MXFInterop_GenericDescriptor_SubDescriptors,  // 272
        MDD_Core_BodySID, // 273
        MDD_Core_IndexSID, // 274
        MDD_Core_OperationalPattern, // 275
        MDD_Core_EssenceContainers, // 276
        MDD_DCAudioChannelCfg_1_5p1, // 277
        MDD_DCAudioChannelCfg_2_6p1, // 278
        MDD_DCAudioChannelCfg_3_7p1, // 279
        MDD_DCAudioChannelCfg_4_WTF, // 280
        MDD_DCAudioChannelCfg_5_7p1_DS, // 281
        MDD_MCALabelSubDescriptor, // 282
        MDD_AudioChannelLabelSubDescriptor, // 283
        MDD_SoundfieldGroupLabelSubDescriptor, // 284
        MDD_GroupOfSoundfieldGroupsLabelSubDescriptor, // 285
        MDD_MCALabelSubDescriptor_MCALabelDictionaryID, // 286
        MDD_MCALabelSubDescriptor_MCALinkID, // 287
        MDD_MCALabelSubDescriptor_MCATagSymbol, // 288
        MDD_MCALabelSubDescriptor_MCATagName, // 289
        MDD_MCALabelSubDescriptor_MCAChannelID, // 290
        MDD_MCALabelSubDescriptor_RFC5646SpokenLanguage, // 291
        MDD_AudioChannelLabelSubDescriptor_SoundfieldGroupLinkID, // 292
        MDD_SoundfieldGroupLabelSubDescriptor_GroupOfSoundfieldGroupsLinkID, // 293
        MDD_DCDataWrappingFrame, // 294
        MDD_DCDataEssence, // 295
        MDD_DCDataDescriptor, // 296
        MDD_DolbyAtmosSubDescriptor, // 297
        MDD_DolbyAtmosSubDescriptor_AtmosVersion, // 298
        MDD_DolbyAtmosSubDescriptor_MaxChannelCount, // 299
        MDD_DolbyAtmosSubDescriptor_MaxObjectCount, // 300
        MDD_DolbyAtmosSubDescriptor_AtmosID, // 301
        MDD_DolbyAtmosSubDescriptor_FirstFrame, // 302
        MDD_DataDataDef, // 303
	MDD_DCAudioChannelCfg_MCA, // 304
	MDD_DCAudioChannel_L, // 305
	MDD_DCAudioChannel_R, // 306
	MDD_DCAudioChannel_C, // 307
	MDD_DCAudioChannel_LFE, // 308
	MDD_DCAudioChannel_Ls, // 309
	MDD_DCAudioChannel_Rs, // 310
	MDD_DCAudioChannel_Lss, // 311
	MDD_DCAudioChannel_Rss, // 312
	MDD_DCAudioChannel_Lrs, // 313
	MDD_DCAudioChannel_Rrs, // 314
	MDD_DCAudioChannel_Lc, // 315
	MDD_DCAudioChannel_Rc, // 316
	MDD_DCAudioChannel_Cs, // 317
	MDD_DCAudioChannel_HI, // 318
	MDD_DCAudioChannel_VIN, // 319
	MDD_DCAudioSoundfield_51, // 320
	MDD_DCAudioSoundfield_71, // 321
	MDD_DCAudioSoundfield_SDS, // 322
	MDD_DCAudioSoundfield_61, // 323
	MDD_DCAudioSoundfield_M, // 324
	MDD_WAVEssenceClip, // 325
	MDD_IMFAudioChannelCfg_MCA, // 326
	MDD_IMFAudioChannel_M1, // 327
	MDD_IMFAudioChannel_M2, // 328
	MDD_IMFAudioChannel_Lt, // 329
	MDD_IMFAudioChannel_Rt, // 330
	MDD_IMFAudioChannel_Lst, // 331
	MDD_IMFAudioChannel_Rst, // 332
	MDD_IMFAudioChannel_S, // 333
	MDD_IMFNumberedSourceChannel, // 334
	MDD_IMFAudioSoundfield_ST, // 335
	MDD_IMFAudioSoundfield_DM, // 336
	MDD_IMFAudioSoundfield_DNS, // 337
	MDD_IMFAudioSoundfield_30, // 338
	MDD_IMFAudioSoundfield_40, // 339
	MDD_IMFAudioSoundfield_50, // 340
	MDD_IMFAudioSoundfield_60, // 341
	MDD_IMFAudioSoundfield_70, // 342
	MDD_IMFAudioSoundfield_LtRt, // 343
	MDD_IMFAudioSoundfield_51Ex, // 344
	MDD_IMFAudioSoundfield_HI, // 345
	MDD_IMFAudioSoundfield_VIN, // 346
	MDD_IMFAudioGroup_MPg, // 347
	MDD_IMFAudioGroup_DVS, // 348
	MDD_IMFAudioGroup_Dcm, // 349
	MDD_MaterialPackage_PackageMarker, // 350
	MDD_GenericPictureEssenceDescriptor_CodingEquations, // 351
	MDD_GenericPictureEssenceDescriptor_ColorPrimaries, // 352
	MDD_JP2KEssenceCompression_BroadcastProfile_1, // 353
	MDD_JP2KEssenceCompression_BroadcastProfile_2, // 354
	MDD_JP2KEssenceCompression_BroadcastProfile_3, // 355
	MDD_JP2KEssenceCompression_BroadcastProfile_4, // 356
	MDD_JP2KEssenceCompression_BroadcastProfile_5, // 357
	MDD_JP2KEssenceCompression_BroadcastProfile_6, // 358
	MDD_JP2KEssenceCompression_BroadcastProfile_7, // 359
	MDD_WaveAudioDescriptor_ReferenceImageEditRate, // 360
	MDD_WaveAudioDescriptor_ReferenceAudioAlignmentLevel, // 361
	MDD_GenericPictureEssenceDescriptor_AlternativeCenterCuts, // 362
	MDD_GenericPictureEssenceDescriptor_ActiveHeight, // 363
	MDD_GenericPictureEssenceDescriptor_ActiveWidth, // 364
	MDD_GenericPictureEssenceDescriptor_ActiveXOffset, // 365
	MDD_GenericPictureEssenceDescriptor_ActiveYOffset, // 366
	MDD_TimedTextDescriptor_RFC5646LanguageTagList, // 367
	MDD_AlternativeCenterCuts_4x3, // 368
	MDD_AlternativeCenterCuts_14x9, // 369
	MDD_WAVWrappingClip, // 370
	MDD_DBOXMotionCodePrimaryStream, // 371
	MDD_DBOXMotionCodeSecondaryStream, // 372
	MDD_ContainerConstraintSubDescriptor, // 373
	MDD_PHDRImageMetadataWrappingFrame, // 374
	MDD_PHDRImageMetadataItem, // 375
	MDD_PHDRMetadataTrackSubDescriptor, // 376
	MDD_PHDRMetadataTrackSubDescriptor_DataDefinition, // 377
	MDD_PHDRMetadataTrackSubDescriptor_SourceTrackID, // 378
	MDD_PHDRMetadataTrackSubDescriptor_SimplePayloadSID, // 379
	MDD_JPEG2000PictureSubDescriptor_J2CLayout, // 380
	MDD_PrivateDCDataWrappingFrame, // 381
	MDD_PrivateDCDataEssence,  // 382
	MDD_PrivateDCDataDescriptor, // 383
    MDD_MCALabelSubDescriptor_MCATitle, // 384
    MDD_MCALabelSubDescriptor_MCATitleVersion, // 385
    MDD_MCALabelSubDescriptor_MCATitleSubVersion, // 386
    MDD_MCALabelSubDescriptor_MCAEpisode, // 387
    MDD_MCALabelSubDescriptor_MCAPartitionKind, // 388
    MDD_MCALabelSubDescriptor_MCAPartitionNumber, // 389
    MDD_MCALabelSubDescriptor_MCAAudioContentKind, // 390
    MDD_MCALabelSubDescriptor_MCAAudioElementKind, // 391
        MDD_Max
    }; // enum MDD_t

    //
    const MDD_t MDD_EssenceContainerData_BodySID = MDD_Core_BodySID;
    const MDD_t MDD_IndexTableSegmentBase_IndexSID = MDD_Core_IndexSID;
    const MDD_t MDD_EssenceContainerData_IndexSID = MDD_Core_IndexSID;
    const MDD_t MDD_DMSegment_DataDefinition = MDD_StructuralComponent_DataDefinition;
    const MDD_t MDD_DMSegment_Duration = MDD_StructuralComponent_Duration;
    const MDD_t MDD_Preface_EssenceContainers = MDD_Core_EssenceContainers;
    const MDD_t MDD_Preface_OperationalPattern = MDD_Core_OperationalPattern;
    const MDD_t MDD_TimedTextResourceSubDescriptor_EssenceStreamID = MDD_Core_BodySID;

} // namespaceASDCP


#endif // _MDD_H_

//
// end MDD.h
//
