#pragma once

#define SAFE_RELEASE(x) { if(x){ x->Release(); x = 0; } }

// used to ensure that TXAA has no aftereffects
class D3D11SavedState
{
public:

	D3D11SavedState(ID3D11DeviceContext *context)
		: pd3dContext(context)
	{
		pd3dContext->AddRef();
		SaveState();
	}

	D3D11SavedState(ID3D11Device *device)
	{
		device->GetImmediateContext(&pd3dContext);
		SaveState();
	}

	~D3D11SavedState()
	{
		// first set NULL RTs in case we have bounds targets that need to go back to SRVs
		ID3D11RenderTargetView *pNULLRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
		for(int i=0;i<D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;i++)
			pNULLRTVs[i] = NULL;
		pd3dContext->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,pNULLRTVs,NULL);

		// Then restore state
		pd3dContext->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pVSSRVs);
		pd3dContext->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pPSSRVs);
		pd3dContext->GSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pGSSRVs);
		pd3dContext->HSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pHSSRVs);
		pd3dContext->DSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pDSSRVs);
		pd3dContext->CSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pCSSRVs);

		UINT NumViewPorts = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
		pd3dContext->RSSetViewports(NumViewPorts,Viewports);
		pd3dContext->VSSetShader(pVertexShader,NULL,NULL);
		pd3dContext->PSSetShader(pPixelShader,NULL,NULL);
		pd3dContext->GSSetShader(pGeometryShader, NULL, NULL);
		pd3dContext->HSSetShader(pHullShader, NULL, NULL);
		pd3dContext->DSSetShader(pDomainShader, NULL, NULL);
		pd3dContext->CSSetShader(pComputeShader, NULL, NULL);

		pd3dContext->VSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pVSSamplers);
		pd3dContext->PSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pPSSamplers);
		pd3dContext->GSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pGSSamplers);
		pd3dContext->HSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pHSSamplers);
		pd3dContext->DSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pDSSamplers);
		pd3dContext->CSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pCSSamplers);
		
		pd3dContext->VSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pVSConstantBuffers);
		pd3dContext->PSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pPSConstantBuffers);
		pd3dContext->GSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pGSConstantBuffers);
		pd3dContext->HSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pHSConstantBuffers);
		pd3dContext->DSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pDSConstantBuffers);
		pd3dContext->CSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pCSConstantBuffers);
		
		pd3dContext->RSSetState(pRasterizer);
		pd3dContext->OMSetDepthStencilState(pDepthStencil,NULL);
		pd3dContext->OMSetBlendState(pBlendState,BlendFactors,BlendSampleMask);
		pd3dContext->IASetInputLayout(pInputLayout);
		pd3dContext->IASetPrimitiveTopology(PrimitiveToplogy);
		//pd3dContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, pRTVs, pDSV, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, D3D11_PS_CS_UAV_REGISTER_COUNT, pUAVs, 0);
		pd3dContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, pRTVs, pDSV, 0, 0, pUAVs, 0);


		SAFE_RELEASE(pd3dContext);	// we added a manual ref

		for(int i=0;i<D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;i++)
			SAFE_RELEASE(pRTVs[i]);
		SAFE_RELEASE(pDSV);
		//for (int i = 0; i < D3D11_PS_CS_UAV_REGISTER_COUNT; i++)
		//	SAFE_RELEASE(pUAVs[i]);


		for(int i=0;i<D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;i++) {
			SAFE_RELEASE(pVSSRVs[i]);
			SAFE_RELEASE(pPSSRVs[i]);
			SAFE_RELEASE(pGSSRVs[i]);
			SAFE_RELEASE(pHSSRVs[i]);
			SAFE_RELEASE(pDSSRVs[i]);
			SAFE_RELEASE(pCSSRVs[i]);
		}
		SAFE_RELEASE(pVertexShader);
		SAFE_RELEASE(pPixelShader);
		SAFE_RELEASE(pGeometryShader);
		SAFE_RELEASE(pHullShader);
		SAFE_RELEASE(pDomainShader);
		SAFE_RELEASE(pComputeShader);

		for(int i=0;i<D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;i++) {
			SAFE_RELEASE(pPSSamplers[i]);
			SAFE_RELEASE(pVSSamplers[i]);
			SAFE_RELEASE(pGSSamplers[i]);
			SAFE_RELEASE(pHSSamplers[i]);
			SAFE_RELEASE(pDSSamplers[i]);
			SAFE_RELEASE(pCSSamplers[i]);
		}                
		for (int i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; i++) {
			SAFE_RELEASE(pVSConstantBuffers[i]);
			SAFE_RELEASE(pPSConstantBuffers[i]);
			SAFE_RELEASE(pGSConstantBuffers[i]);
			SAFE_RELEASE(pHSConstantBuffers[i]);
			SAFE_RELEASE(pDSConstantBuffers[i]);
			SAFE_RELEASE(pCSConstantBuffers[i]);
		}
		for(int i=0;i<D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;i++)
			SAFE_RELEASE(pPSConstantBuffers[i]);
		SAFE_RELEASE(pRasterizer);
		SAFE_RELEASE(pDepthStencil);
		SAFE_RELEASE(pBlendState);
		SAFE_RELEASE(pInputLayout);
	}

private:

	void SaveState()
	{
		pd3dContext->OMGetRenderTargetsAndUnorderedAccessViews(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, pRTVs, &pDSV, 0, 0, pUAVs);
		//pd3dContext->OMGetRenderTargetsAndUnorderedAccessViews(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, pRTVs, &pDSV, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, D3D11_PS_CS_UAV_REGISTER_COUNT, pUAVs);
		pd3dContext->VSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pVSSRVs);
		pd3dContext->PSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pPSSRVs);
		pd3dContext->GSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pGSSRVs);
		pd3dContext->HSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pHSSRVs);
		pd3dContext->DSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pDSSRVs);
		pd3dContext->CSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pCSSRVs);
		
		UINT NumViewPorts = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
		pd3dContext->RSGetViewports(&NumViewPorts,Viewports);
		pd3dContext->VSGetShader(&pVertexShader,NULL,NULL);
		pd3dContext->PSGetShader(&pPixelShader,NULL,NULL);
		pd3dContext->GSGetShader(&pGeometryShader, NULL, NULL);
		pd3dContext->HSGetShader(&pHullShader, NULL, NULL);
		pd3dContext->DSGetShader(&pDomainShader, NULL, NULL);
		pd3dContext->CSGetShader(&pComputeShader, NULL, NULL);

		pd3dContext->VSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pVSSamplers);
		pd3dContext->PSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pPSSamplers);
		pd3dContext->GSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pGSSamplers);
		pd3dContext->HSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pHSSamplers);
		pd3dContext->DSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pDSSamplers);
		pd3dContext->CSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pCSSamplers);

		pd3dContext->VSGetConstantBuffers(0,D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,  pVSConstantBuffers);
		pd3dContext->PSGetConstantBuffers(0,D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,  pPSConstantBuffers);
		pd3dContext->GSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pGSConstantBuffers);
		pd3dContext->HSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pHSConstantBuffers);
		pd3dContext->DSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pDSConstantBuffers);
		pd3dContext->CSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pCSConstantBuffers);

		pd3dContext->RSGetState(&pRasterizer);
		pd3dContext->OMGetDepthStencilState(&pDepthStencil,NULL);
		pd3dContext->OMGetBlendState(&pBlendState,BlendFactors,&BlendSampleMask);
		pd3dContext->IAGetInputLayout(&pInputLayout);
		pd3dContext->IAGetPrimitiveTopology(&PrimitiveToplogy);
	}

	ID3D11RenderTargetView *pRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];  
	ID3D11UnorderedAccessView *pUAVs[D3D11_PS_CS_UAV_REGISTER_COUNT];
	UINT pUAVInitialCounts[D3D11_PS_CS_UAV_REGISTER_COUNT];

	ID3D11DepthStencilView*pDSV;
	ID3D11ShaderResourceView*pVSSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11ShaderResourceView*pPSSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11ShaderResourceView*pGSSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11ShaderResourceView*pHSSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11ShaderResourceView*pDSSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11ShaderResourceView*pCSSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];

	D3D11_VIEWPORT Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	ID3D11VertexShader  *pVertexShader;
	ID3D11PixelShader   *pPixelShader;
	ID3D11GeometryShader *pGeometryShader;
	ID3D11HullShader     *pHullShader;
	ID3D11DomainShader   *pDomainShader;
	ID3D11ComputeShader  *pComputeShader;

	ID3D11SamplerState*pVSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11SamplerState*pPSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11SamplerState*pGSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11SamplerState*pHSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11SamplerState*pDSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11SamplerState*pCSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];

	ID3D11Buffer *pVSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *pPSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *pGSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *pHSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *pDSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *pCSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];

	ID3D11RasterizerState *pRasterizer;
	ID3D11DepthStencilState *pDepthStencil;
	ID3D11BlendState *pBlendState;
	FLOAT BlendFactors[4];
	UINT BlendSampleMask;
	ID3D11InputLayout *pInputLayout;
	D3D11_PRIMITIVE_TOPOLOGY PrimitiveToplogy;

	ID3D11DeviceContext *pd3dContext;
};
