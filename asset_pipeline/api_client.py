"""
AI Image Generation API Client

Unified client interface for multiple AI image generation APIs.
Each provider implements the same interface, allowing easy switching
between Leonardo.ai, OpenAI, and Stability AI.

Recommended: Leonardo.ai
  - Best consistency for game art styles
  - Good API with generation history
  - Supports fine-tuned models for specific art styles
  - Cost-effective for batch generation
  - Has built-in background removal
"""

import time
import json
import logging
from abc import ABC, abstractmethod
from typing import Optional
from dataclasses import dataclass

from .config import PipelineConfig, APIProvider, ImageSpec

logger = logging.getLogger(__name__)


@dataclass
class GenerationResult:
    """Result from an image generation request."""
    success: bool
    image_data: Optional[bytes] = None       # Raw image bytes
    image_url: Optional[str] = None          # URL to download
    generation_id: Optional[str] = None      # Provider-specific ID
    prompt_used: str = ""
    error_message: str = ""
    metadata: dict = None

    def __post_init__(self):
        if self.metadata is None:
            self.metadata = {}


class ImageGeneratorBase(ABC):
    """Abstract base class for AI image generation providers."""

    def __init__(self, config: PipelineConfig):
        self.config = config

    @abstractmethod
    def generate(self, prompt: str, negative_prompt: str = "",
                 spec: Optional[ImageSpec] = None) -> GenerationResult:
        """Generate a single image from a text prompt."""
        pass

    @abstractmethod
    def generate_variation(self, image_data: bytes, prompt: str,
                           spec: Optional[ImageSpec] = None) -> GenerationResult:
        """Generate a variation of an existing image."""
        pass

    @abstractmethod
    def check_status(self, generation_id: str) -> GenerationResult:
        """Check the status of an async generation."""
        pass

    @abstractmethod
    def download_image(self, url: str) -> bytes:
        """Download a generated image from its URL."""
        pass

    def _rate_limit(self):
        """Apply rate limiting between API calls."""
        time.sleep(self.config.rate_limit_delay)


class LeonardoClient(ImageGeneratorBase):
    """
    Leonardo.ai API Client

    Leonardo.ai is the recommended provider for this project because:
    1. Consistent style across multiple generations
    2. Supports custom fine-tuned models for specific game art styles
    3. Built-in background removal (alchemy)
    4. Good batch processing support
    5. Reasonable pricing ($10/mo for 8500 tokens)

    API Docs: https://docs.leonardo.ai/reference
    """

    API_BASE = "https://cloud.leonardo.ai/api/rest/v1"

    def __init__(self, config: PipelineConfig):
        super().__init__(config)
        self.headers = {
            "Authorization": f"Bearer {config.api_key}",
            "Content-Type": "application/json",
            "Accept": "application/json",
        }

    def generate(self, prompt: str, negative_prompt: str = "",
                 spec: Optional[ImageSpec] = None) -> GenerationResult:
        """Generate image using Leonardo.ai API."""
        try:
            import requests
        except ImportError:
            return GenerationResult(
                success=False,
                error_message="requests library required: pip install requests"
            )

        if spec is None:
            spec = ImageSpec()

        payload = {
            "prompt": prompt,
            "negative_prompt": negative_prompt or self._default_negative(),
            "modelId": self.config.leonardo_model_id,
            "width": spec.width,
            "height": spec.height,
            "num_images": spec.num_variants,
            "presetStyle": self.config.leonardo_preset,
            "alchemy": True,            # Better quality
            "transparency": "foreground_only" if spec.transparent_background else "disabled",
            "highResolution": True,
        }

        if spec.seed is not None:
            payload["seed"] = spec.seed

        try:
            self._rate_limit()
            response = requests.post(
                f"{self.API_BASE}/generations",
                headers=self.headers,
                json=payload,
                timeout=30
            )
            response.raise_for_status()
            data = response.json()

            generation_id = data.get("sdGenerationJob", {}).get("generationId")
            if not generation_id:
                return GenerationResult(
                    success=False,
                    error_message="No generation ID in response"
                )

            # Poll for completion
            return self._wait_for_generation(generation_id, requests)

        except Exception as e:
            logger.error(f"Leonardo generation failed: {e}")
            return GenerationResult(
                success=False,
                error_message=str(e),
                prompt_used=prompt
            )

    def _wait_for_generation(self, generation_id: str, requests_mod) -> GenerationResult:
        """Poll Leonardo API until generation is complete."""
        max_attempts = 60
        poll_interval = 2.0

        for attempt in range(max_attempts):
            time.sleep(poll_interval)
            try:
                response = requests_mod.get(
                    f"{self.API_BASE}/generations/{generation_id}",
                    headers=self.headers,
                    timeout=15
                )
                response.raise_for_status()
                data = response.json()

                gen_data = data.get("generations_by_pk", {})
                status = gen_data.get("status")

                if status == "COMPLETE":
                    images = gen_data.get("generated_images", [])
                    if images:
                        image_url = images[0].get("url")
                        return GenerationResult(
                            success=True,
                            image_url=image_url,
                            generation_id=generation_id,
                            metadata={"all_images": [img.get("url") for img in images]}
                        )

                elif status == "FAILED":
                    return GenerationResult(
                        success=False,
                        error_message="Generation failed on Leonardo servers",
                        generation_id=generation_id
                    )

            except Exception as e:
                logger.warning(f"Poll attempt {attempt} failed: {e}")
                continue

        return GenerationResult(
            success=False,
            error_message="Generation timed out",
            generation_id=generation_id
        )

    def generate_variation(self, image_data: bytes, prompt: str,
                           spec: Optional[ImageSpec] = None) -> GenerationResult:
        """Generate a variation using Leonardo's image-to-image."""
        # Upload init image first, then generate with it
        return GenerationResult(
            success=False,
            error_message="Variation generation not yet implemented for Leonardo"
        )

    def check_status(self, generation_id: str) -> GenerationResult:
        """Check status of a Leonardo generation."""
        try:
            import requests
            return self._wait_for_generation(generation_id, requests)
        except Exception as e:
            return GenerationResult(success=False, error_message=str(e))

    def download_image(self, url: str) -> bytes:
        """Download image from Leonardo CDN."""
        import requests
        response = requests.get(url, timeout=30)
        response.raise_for_status()
        return response.content

    @staticmethod
    def _default_negative() -> str:
        return (
            "blurry, low quality, text, watermark, signature, "
            "realistic, photographic, 3d render, gradient background, "
            "noisy, jpeg artifacts, cropped, out of frame"
        )


class OpenAIClient(ImageGeneratorBase):
    """
    OpenAI Image Generation Client (DALL-E / GPT Image)

    Good for: Complex prompt understanding, consistent style with
    detailed instructions. More expensive per image than Leonardo.
    """

    def __init__(self, config: PipelineConfig):
        super().__init__(config)

    def generate(self, prompt: str, negative_prompt: str = "",
                 spec: Optional[ImageSpec] = None) -> GenerationResult:
        """Generate image using OpenAI API."""
        try:
            from openai import OpenAI
        except ImportError:
            return GenerationResult(
                success=False,
                error_message="openai library required: pip install openai"
            )

        if spec is None:
            spec = ImageSpec()

        client = OpenAI(api_key=self.config.api_key)

        # OpenAI uses specific size options
        size = self._map_size(spec.width, spec.height)

        try:
            self._rate_limit()
            response = client.images.generate(
                model=self.config.openai_model,
                prompt=prompt,
                n=spec.num_variants,
                size=size,
                quality=self.config.openai_quality,
                response_format="url",
            )

            image_url = response.data[0].url
            return GenerationResult(
                success=True,
                image_url=image_url,
                prompt_used=prompt,
                metadata={
                    "all_urls": [d.url for d in response.data]
                }
            )

        except Exception as e:
            logger.error(f"OpenAI generation failed: {e}")
            return GenerationResult(
                success=False,
                error_message=str(e),
                prompt_used=prompt
            )

    def generate_variation(self, image_data: bytes, prompt: str,
                           spec: Optional[ImageSpec] = None) -> GenerationResult:
        """Generate variation using OpenAI edit endpoint."""
        return GenerationResult(
            success=False,
            error_message="Variation generation not yet implemented for OpenAI"
        )

    def check_status(self, generation_id: str) -> GenerationResult:
        """OpenAI generations are synchronous - no status to check."""
        return GenerationResult(success=True)

    def download_image(self, url: str) -> bytes:
        import requests
        response = requests.get(url, timeout=30)
        response.raise_for_status()
        return response.content

    @staticmethod
    def _map_size(width: int, height: int) -> str:
        if width == height:
            return "1024x1024"
        elif width > height:
            return "1792x1024"
        else:
            return "1024x1792"


class StabilityClient(ImageGeneratorBase):
    """
    Stability AI (Stable Diffusion) Client

    Good for: Fine-grained control, inpainting, specific style models.
    Supports ControlNet for consistent sprite poses.
    """

    API_BASE = "https://api.stability.ai/v2beta"

    def __init__(self, config: PipelineConfig):
        super().__init__(config)
        self.headers = {
            "Authorization": f"Bearer {config.api_key}",
            "Accept": "image/*",
        }

    def generate(self, prompt: str, negative_prompt: str = "",
                 spec: Optional[ImageSpec] = None) -> GenerationResult:
        """Generate image using Stability AI API."""
        try:
            import requests
        except ImportError:
            return GenerationResult(
                success=False,
                error_message="requests library required: pip install requests"
            )

        if spec is None:
            spec = ImageSpec()

        payload = {
            "prompt": prompt,
            "negative_prompt": negative_prompt,
            "output_format": spec.format,
            "aspect_ratio": "1:1",
        }

        if spec.seed is not None:
            payload["seed"] = spec.seed

        try:
            self._rate_limit()
            response = requests.post(
                f"{self.API_BASE}/stable-image/generate/sd3",
                headers={**self.headers, "Content-Type": "multipart/form-data"},
                files={"none": ""},
                data=payload,
                timeout=60
            )
            response.raise_for_status()

            return GenerationResult(
                success=True,
                image_data=response.content,
                prompt_used=prompt,
            )

        except Exception as e:
            logger.error(f"Stability generation failed: {e}")
            return GenerationResult(
                success=False,
                error_message=str(e),
                prompt_used=prompt
            )

    def generate_variation(self, image_data: bytes, prompt: str,
                           spec: Optional[ImageSpec] = None) -> GenerationResult:
        return GenerationResult(
            success=False,
            error_message="Variation generation not yet implemented for Stability"
        )

    def check_status(self, generation_id: str) -> GenerationResult:
        return GenerationResult(success=True)

    def download_image(self, url: str) -> bytes:
        import requests
        response = requests.get(url, timeout=30)
        response.raise_for_status()
        return response.content


def create_client(config: PipelineConfig) -> ImageGeneratorBase:
    """Factory function to create the appropriate API client."""
    clients = {
        APIProvider.LEONARDO: LeonardoClient,
        APIProvider.OPENAI: OpenAIClient,
        APIProvider.STABILITY: StabilityClient,
    }

    client_class = clients.get(config.api_provider)
    if not client_class:
        raise ValueError(f"Unsupported API provider: {config.api_provider}")

    return client_class(config)
