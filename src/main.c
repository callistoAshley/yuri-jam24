#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

int main(int argc, char *argv[])
{
    VkInstance vulkan;
    const VkInstanceCreateInfo inst_info =
    {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        NULL,
        0,
        NULL,
        0,
        NULL,
        0,
        NULL
    };

    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
    vkCreateInstance(&inst_info, NULL, &vulkan);
    vkDestroyInstance(vulkan, NULL);
    SDL_Quit();
}
