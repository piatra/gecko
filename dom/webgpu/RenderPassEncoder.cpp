/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/WebGPUBinding.h"
#include "RenderPassEncoder.h"
#include "BindGroup.h"
#include "CommandEncoder.h"
#include "RenderPipeline.h"

namespace mozilla {
namespace webgpu {

GPU_IMPL_CYCLE_COLLECTION(RenderPassEncoder, mParent, mUsedBindGroups,
                          mUsedBuffers, mUsedPipelines, mUsedTextureViews)
GPU_IMPL_JS_WRAP(RenderPassEncoder)

ffi::WGPURenderPass* ScopedFfiRenderTraits::empty() { return nullptr; }

void ScopedFfiRenderTraits::release(ffi::WGPURenderPass* raw) {
  if (raw) {
    ffi::wgpu_render_pass_destroy(raw);
  }
}

ffi::WGPULoadOp ConvertLoadOp(const dom::GPULoadOp& aOp) {
  switch (aOp) {
    case dom::GPULoadOp::Load:
      return ffi::WGPULoadOp_Load;
    default:
      MOZ_CRASH("Unexpected load op");
  }
}

ffi::WGPUStoreOp ConvertStoreOp(const dom::GPUStoreOp& aOp) {
  switch (aOp) {
    case dom::GPUStoreOp::Store:
      return ffi::WGPUStoreOp_Store;
    case dom::GPUStoreOp::Clear:
      return ffi::WGPUStoreOp_Clear;
    default:
      MOZ_CRASH("Unexpected load op");
  }
}

ffi::WGPUColor ConvertColor(const dom::GPUColorDict& aColor) {
  ffi::WGPUColor color = {aColor.mR, aColor.mG, aColor.mB, aColor.mA};
  return color;
}

ffi::WGPURenderPass* BeginRenderPass(
    RawId aEncoderId, const dom::GPURenderPassDescriptor& aDesc) {
  ffi::WGPURenderPassDescriptor desc = {};

  ffi::WGPUDepthStencilAttachmentDescriptor dsDesc = {};
  if (aDesc.mDepthStencilAttachment.WasPassed()) {
    const auto& dsa = aDesc.mDepthStencilAttachment.Value();
    dsDesc.attachment = dsa.mAttachment->mId;

    if (dsa.mDepthLoadValue.IsFloat()) {
      dsDesc.depth.load_op = ffi::WGPULoadOp_Clear;
      dsDesc.depth.clear_value = dsa.mDepthLoadValue.GetAsFloat();
    }
    if (dsa.mDepthLoadValue.IsGPULoadOp()) {
      dsDesc.depth.load_op =
          ConvertLoadOp(dsa.mDepthLoadValue.GetAsGPULoadOp());
    }
    dsDesc.depth.store_op = ConvertStoreOp(dsa.mDepthStoreOp);

    if (dsa.mStencilLoadValue.IsRangeEnforcedUnsignedLong()) {
      dsDesc.stencil.load_op = ffi::WGPULoadOp_Clear;
      dsDesc.stencil.clear_value =
          dsa.mStencilLoadValue.GetAsRangeEnforcedUnsignedLong();
    }
    if (dsa.mStencilLoadValue.IsGPULoadOp()) {
      dsDesc.stencil.load_op =
          ConvertLoadOp(dsa.mStencilLoadValue.GetAsGPULoadOp());
    }
    dsDesc.stencil.store_op = ConvertStoreOp(dsa.mStencilStoreOp);

    desc.depth_stencil_attachment = &dsDesc;
  }

  std::array<ffi::WGPUColorAttachmentDescriptor, WGPUMAX_COLOR_TARGETS>
      colorDescs = {};
  desc.color_attachments = colorDescs.data();
  desc.color_attachments_length = aDesc.mColorAttachments.Length();

  for (size_t i = 0; i < aDesc.mColorAttachments.Length(); ++i) {
    const auto& ca = aDesc.mColorAttachments[i];
    ffi::WGPUColorAttachmentDescriptor& cd = colorDescs[i];
    cd.attachment = ca.mAttachment->mId;
    cd.channel.store_op = ConvertStoreOp(ca.mStoreOp);

    if (ca.mResolveTarget.WasPassed()) {
      cd.resolve_target = ca.mResolveTarget.Value().mId;
    }
    if (ca.mLoadValue.IsGPULoadOp()) {
      cd.channel.load_op = ConvertLoadOp(ca.mLoadValue.GetAsGPULoadOp());
    } else {
      cd.channel.load_op = ffi::WGPULoadOp_Clear;
      if (ca.mLoadValue.IsDoubleSequence()) {
        const auto& seq = ca.mLoadValue.GetAsDoubleSequence();
        if (seq.Length() >= 1) {
          cd.channel.clear_value.r = seq[0];
        }
        if (seq.Length() >= 2) {
          cd.channel.clear_value.g = seq[1];
        }
        if (seq.Length() >= 3) {
          cd.channel.clear_value.b = seq[2];
        }
        if (seq.Length() >= 4) {
          cd.channel.clear_value.a = seq[3];
        }
      }
      if (ca.mLoadValue.IsGPUColorDict()) {
        cd.channel.clear_value =
            ConvertColor(ca.mLoadValue.GetAsGPUColorDict());
      }
    }
  }

  return ffi::wgpu_command_encoder_begin_render_pass(aEncoderId, &desc);
}

RenderPassEncoder::RenderPassEncoder(CommandEncoder* const aParent,
                                     const dom::GPURenderPassDescriptor& aDesc)
    : ChildOf(aParent), mPass(BeginRenderPass(aParent->mId, aDesc)) {
  for (const auto& at : aDesc.mColorAttachments) {
    mUsedTextureViews.AppendElement(at.mAttachment);
  }
  if (aDesc.mDepthStencilAttachment.WasPassed()) {
    mUsedTextureViews.AppendElement(
        aDesc.mDepthStencilAttachment.Value().mAttachment);
  }
}

RenderPassEncoder::~RenderPassEncoder() {
  if (mValid) {
    mValid = false;
  }
}

void RenderPassEncoder::SetBindGroup(
    uint32_t aSlot, const BindGroup& aBindGroup,
    const dom::Sequence<uint32_t>& aDynamicOffsets) {
  if (mValid) {
    mUsedBindGroups.AppendElement(&aBindGroup);
    ffi::wgpu_render_pass_set_bind_group(mPass, aSlot, aBindGroup.mId,
                                         aDynamicOffsets.Elements(),
                                         aDynamicOffsets.Length());
  }
}

void RenderPassEncoder::SetPipeline(const RenderPipeline& aPipeline) {
  if (mValid) {
    mUsedPipelines.AppendElement(&aPipeline);
    ffi::wgpu_render_pass_set_pipeline(mPass, aPipeline.mId);
  }
}

void RenderPassEncoder::SetIndexBuffer(const Buffer& aBuffer, uint64_t aOffset,
                                       uint64_t aSize) {
  if (mValid) {
    mUsedBuffers.AppendElement(&aBuffer);
    ffi::wgpu_render_pass_set_index_buffer(mPass, aBuffer.mId, aOffset, aSize);
  }
}

void RenderPassEncoder::SetVertexBuffer(uint32_t aSlot, const Buffer& aBuffer,
                                        uint64_t aOffset, uint64_t aSize) {
  if (mValid) {
    mUsedBuffers.AppendElement(&aBuffer);
    ffi::wgpu_render_pass_set_vertex_buffer(mPass, aSlot, aBuffer.mId, aOffset,
                                            aSize);
  }
}

void RenderPassEncoder::Draw(uint32_t aVertexCount, uint32_t aInstanceCount,
                             uint32_t aFirstVertex, uint32_t aFirstInstance) {
  if (mValid) {
    ffi::wgpu_render_pass_draw(mPass, aVertexCount, aInstanceCount,
                               aFirstVertex, aFirstInstance);
  }
}

void RenderPassEncoder::DrawIndexed(uint32_t aIndexCount,
                                    uint32_t aInstanceCount,
                                    uint32_t aFirstIndex, int32_t aBaseVertex,
                                    uint32_t aFirstInstance) {
  if (mValid) {
    ffi::wgpu_render_pass_draw_indexed(mPass, aIndexCount, aInstanceCount,
                                       aFirstIndex, aBaseVertex,
                                       aFirstInstance);
  }
}

void RenderPassEncoder::DrawIndirect(const Buffer& aIndirectBuffer,
                                     uint64_t aIndirectOffset) {
  if (mValid) {
    ffi::wgpu_render_pass_draw_indirect(mPass, aIndirectBuffer.mId,
                                        aIndirectOffset);
  }
}

void RenderPassEncoder::DrawIndexedIndirect(const Buffer& aIndirectBuffer,
                                            uint64_t aIndirectOffset) {
  if (mValid) {
    ffi::wgpu_render_pass_draw_indexed_indirect(mPass, aIndirectBuffer.mId,
                                                aIndirectOffset);
  }
}

void RenderPassEncoder::EndPass(ErrorResult& aRv) {
  if (mValid) {
    mValid = false;
    auto* pass = mPass.forget();
    MOZ_ASSERT(pass);
    mParent->EndRenderPass(*pass, aRv);
  }
}

}  // namespace webgpu
}  // namespace mozilla
