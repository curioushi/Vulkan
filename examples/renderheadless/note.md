create instance
select physical device
find a suitable queue family index
create logical device, get device queue
create command pool
copy data to device local buffer
    copy vertices data
        create staging buffer (host visible), copy vertices data
        create vertex buffer (device local)
        submit copy command from staging buffer to vertex buffer
        destroy staging buffer
    copy indices data
        create staging buffer (host visible), copy indices data
        create indices buffer (device local)
        submit copy command from staging buffer to vertex buffer
        destroy staging buffer
create device local image
    create color image & image view
    create depth stencil image & image view
create renderpass
create framebuffer of renderpass
create graphics pipeline (set many state & shader modules)
record command buffer -> submit -> wait
create host visible image
copy image from device to host
