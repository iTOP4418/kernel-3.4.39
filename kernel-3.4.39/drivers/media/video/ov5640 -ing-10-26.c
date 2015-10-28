
/*
 * OV5640 Camera Driver
 *
 * Copyright (C) 2011 Actions Semiconductor Co.,LTD
 * Wang Xin <wangxin@actions-semi.com>
 *
 *
 * Copyright (C) 2008 Renesas Solutions Corp.
 * Kuninori Morimoto <morimoto.kuninori@renesas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * fixed by swpark@nexell.co.kr for compatibility with general v4l2 layer (not using soc camera interface)
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>


#include "ov5640.h"


#define MODULE_NAME "OV5640"
/* private ctrls */
#define V4L2_CID_SCENE_EXPOSURE         (V4L2_CTRL_CLASS_CAMERA | 0x1001)
#define V4L2_CID_PRIVATE_PREV_CAPT      (V4L2_CTRL_CLASS_CAMERA | 0x1002)
#define NUM_CTRLS           11


static inline struct v4l2_subdev *ov5640_ctrl_to_sd(struct v4l2_ctrl *ctrl)
{
    return &container_of(ctrl->handler, struct ov5640_priv, hdl)->subdev;
}

static int ov5640_set_params(struct v4l2_subdev *sd, u32 *width, u32 *height, enum v4l2_mbus_pixelcode code)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct ov5640_priv *priv = to_ov5640(client);
    const struct ov5640_win_size *old_win, *new_win;
    int i;

    /*
     * select format
     */
    priv->cfmt = NULL;
    for (i = 0; i < ARRAY_SIZE(ov5640_cfmts); i++) {
        if (code == ov5640_cfmts[i].code) {
            priv->cfmt = ov5640_cfmts + i;
            break;
        }
    }
    if (!priv->cfmt) {
        printk(KERN_ERR "Unsupported sensor format.\n");
        return -EINVAL;
    }

    /*
     * select win
     */
    old_win = priv->win;
    new_win = select_win(*width, *height);
    if (!new_win) {
        printk(KERN_ERR "Unsupported win size\n");
        return -EINVAL;
    }
    priv->win = new_win;

    priv->rect.left = 0;
    priv->rect.top = 0;
    priv->rect.width = priv->win->width;
    priv->rect.height = priv->win->height;

    *width = priv->win->width;
    *height = priv->win->height;

     printk("%s....%d\n",__FUNCTION__,__LINE__);

    return 0;
}

/****************************************************************************************
 * control functions
 */

static int ov5640_s_ctrl(struct v4l2_ctrl *ctrl)
{
    struct v4l2_subdev *sd = ov5640_ctrl_to_sd(ctrl);
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    int ret = 0;

    OV_INFO("%s: control ctrl- %s\n", __func__, v4l2_ctrl_get_name(ctrl->id));

    return;

    switch (ctrl->id) {
        case V4L2_CID_BRIGHTNESS:
            ov5640_set_brightness(sd, ctrl->val);
            break;

        case V4L2_CID_CONTRAST:
            ov5640_set_contrast(sd, ctrl->val);
            break;

        case V4L2_CID_AUTO_WHITE_BALANCE:
         //   ov5640_set_awb(sd, ctrl->value);
            break;

        case V4L2_CID_EXPOSURE:
            ov5640_set_exposure_level(sd, ctrl->val);
            break;

        case V4L2_CID_GAIN:
          //    ov5640_set_gain(sd, ctrl->val);
            break;

        case V4L2_CID_HFLIP:
             ov5640_set_flip(client, ctrl);
            break;

        case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
          //  set_white_balance_temperature(client, ctrl->val);
            break;

        case V4L2_CID_COLORFX:
             ov5640_set_colorfx(client, ctrl->val);
            break;

        case V4L2_CID_EXPOSURE_AUTO:
           //  ov5640_set_exposure_auto(sd, ctrl->val);
            break;

        case V4L2_CID_SCENE_EXPOSURE:
           //  ov5640_set_scene_exposure(sd, ctrl->val);
            break;

        case V4L2_CID_PRIVATE_PREV_CAPT:
           // sp2518_set_prev_capt_mode(sd, ctrl);
            break;

        default:
            dev_err(&client->dev, "%s: invalid control id %d\n", __func__, ctrl->id);
            return -EINVAL;
    }

    return ret;
}




static const struct v4l2_ctrl_ops ov5640_ctrl_ops = {
    .s_ctrl = ov5640_s_ctrl,
};

static const struct v4l2_ctrl_config ov5640_custom_ctrls[] = {
    {
        .ops    = &ov5640_ctrl_ops,
        .id     = V4L2_CID_SCENE_EXPOSURE,
        .type   = V4L2_CTRL_TYPE_INTEGER,
        .name   = "SceneExposure",
        .min    = 0,
        .max    = 1,
        .def    = 0,
        .step   = 1,
    }, {
        .ops    = &ov5640_ctrl_ops,
        .id     = V4L2_CID_PRIVATE_PREV_CAPT,
        .type   = V4L2_CTRL_TYPE_INTEGER,
        .name   = "PrevCapt",
        .min    = 0,
        .max    = 1,
        .def    = 0,
        .step   = 1,
    }
};

static int ov5640_initialize_ctrls(struct ov5640_priv *priv)
{
    v4l2_ctrl_handler_init(&priv->hdl, NUM_CTRLS);

    /* standard ctrls */
    priv->pbrightness = v4l2_ctrl_new_std(&priv->hdl, &ov5640_ctrl_ops,
            V4L2_CID_BRIGHTNESS, 0, 6, 1, 0);
    if (!priv->pbrightness) {
        printk(KERN_ERR "%s: failed to create brightness ctrl\n", __func__);
        return -ENOENT;
    }

    priv->pcontrast = v4l2_ctrl_new_std(&priv->hdl, &ov5640_ctrl_ops,
            V4L2_CID_CONTRAST, -6, 6, 1, 0);
    if (!priv->pcontrast) {
        printk(KERN_ERR "%s: failed to create contrast ctrl\n", __func__);
        return -ENOENT;
    }

    priv->pauto_white_balance = v4l2_ctrl_new_std(&priv->hdl, &ov5640_ctrl_ops,
            V4L2_CID_AUTO_WHITE_BALANCE, 0, 1, 1, 1);
    if (!priv->pauto_white_balance) {
        printk(KERN_ERR "%s: failed to create auto_white_balance ctrl\n", __func__);
        return -ENOENT;
    }

#if 0
    priv->pexposure = v4l2_ctrl_new_std(&priv->hdl, &ov5640_ctrl_ops,
            V4L2_CID_EXPOSURE, 0, 0xFFFF, 1, 500);
    if (!priv->pexposure) {
        printk(KERN_ERR "%s: failed to create exposure ctrl\n", __func__);
        return -ENOENT;
    }
#endif

    priv->pgain = v4l2_ctrl_new_std(&priv->hdl, &ov5640_ctrl_ops,
            V4L2_CID_GAIN, 0, 0xFF, 1, 128);
    if (!priv->pgain) {
        printk(KERN_ERR "%s: failed to create gain ctrl\n", __func__);
        return -ENOENT;
    }

    priv->phflip = v4l2_ctrl_new_std(&priv->hdl, &ov5640_ctrl_ops,
            V4L2_CID_HFLIP, 0, 1, 1, 0);
    if (!priv->phflip) {
        printk(KERN_ERR "%s: failed to create hflip ctrl\n", __func__);
        return -ENOENT;
    }

    priv->pwhite_balance_temperature = v4l2_ctrl_new_std(&priv->hdl, &ov5640_ctrl_ops,
            V4L2_CID_WHITE_BALANCE_TEMPERATURE, 0, 3, 1, 1);
    if (!priv->pwhite_balance_temperature) {
        printk(KERN_ERR "%s: failed to create white_balance_temperature ctrl\n", __func__);
        return -ENOENT;
    }

    /* standard menus */
    priv->pcolorfx = v4l2_ctrl_new_std_menu(&priv->hdl, &ov5640_ctrl_ops,
            V4L2_CID_COLORFX, 3, 0, 0);
    if (!priv->pcolorfx) {
        printk(KERN_ERR "%s: failed to create colorfx ctrl\n", __func__);
        return -ENOENT;
    }

    priv->pexposure_auto = v4l2_ctrl_new_std_menu(&priv->hdl, &ov5640_ctrl_ops,
            V4L2_CID_EXPOSURE_AUTO, 1, 0, 1);
    if (!priv->pexposure_auto) {
        printk(KERN_ERR "%s: failed to create exposure_auto ctrl\n", __func__);
        return -ENOENT;
    }

    /* custom ctrls */
    priv->pscene_exposure = v4l2_ctrl_new_custom(&priv->hdl, &ov5640_custom_ctrls[0], NULL);
    if (!priv->pscene_exposure) {
        printk(KERN_ERR "%s: failed to create scene_exposure ctrl\n", __func__);
        return -ENOENT;
    }

    priv->pprev_capt = v4l2_ctrl_new_custom(&priv->hdl, &ov5640_custom_ctrls[1], NULL);
    if (!priv->pprev_capt) {
        printk(KERN_ERR "%s: failed to create prev_capt ctrl\n", __func__);
        return -ENOENT;
    }

    priv->subdev.ctrl_handler = &priv->hdl;
    if (priv->hdl.error) {
        printk(KERN_ERR "%s: ctrl handler error(%d)\n", __func__, priv->hdl.error);
        v4l2_ctrl_handler_free(&priv->hdl);
        return -EINVAL;
    }


    printk("%s..................\n", __func__);

    return 0;
}





/****************************************************************************************
 * v4l2 subdev ops
 */

/**
 * core ops
 */
static int ov5640_g_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *id)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct ov5640_priv *priv = to_ov5640(client);
    id->ident    = priv->model;
    id->revision = 0;


    printk("%s: on  line %d \n", __func__, __LINE__);


    return 0;
}

static int ov5640_s_power(struct v4l2_subdev *sd, int on)
{
    /* used when suspending */
    printk("%s: on %d\n", __func__, on);
    if (!on) {

        struct i2c_client * client = v4l2_get_subdevdata(sd);
        struct ov5640_priv *priv = to_ov5640(client);
        priv->initialized = false;
    }

    return 0;
}

static const struct v4l2_subdev_core_ops ov5640_subdev_core_ops = {
    .g_chip_ident	=  ov5640_g_chip_ident ,//sp2518_g_chip_ident,
    .s_power        = ov5640_s_power,//sp2518_s_power,
    .s_ctrl         = v4l2_subdev_s_ctrl,
};

/**
 * video ops
 */

static int ov5640_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_frmsizeenum *fsize)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    printk("%s.............%d...........\n",__func__,__LINE__);

    if (fsize->index >= ARRAY_SIZE(ov5640_wins)) {
        dev_err(&client->dev, "index(%d) is over range %d  %s\n", fsize->index, ARRAY_SIZE(ov5640_wins),__func__);
        return -EINVAL;
    }

    switch (fsize->pixel_format) {
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_YUV422P:
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_YUYV:
            fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
            fsize->discrete.width = 640;//ov5645_win[fsize->index]->width;
            fsize->discrete.height = 480;//ov5645_win[fsize->index]->height;
            break;
        default:
            dev_err(&client->dev, "pixel_format(%d) is Unsupported %s \n", fsize->pixel_format, __func__);
            return -EINVAL;
    }

     //printk("type %d, width %d, height %d\n", V4L2_FRMSIZE_TYPE_DISCRETE, fsize->discrete.width, fsize->discrete.height);

     printk("%s,%d,type %d, width %d, height %d\n", __func__,__LINE__, V4L2_FRMSIZE_TYPE_DISCRETE, fsize->discrete.width, fsize->discrete.height);

    return 0;
}



static int ov5640_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned int index,
        enum v4l2_mbus_pixelcode *code)
{
    if (index >= ARRAY_SIZE(ov5640_cfmts))
        return -EINVAL;

    *code = ov5640_cfmts[index].code;


    printk("%s, %d   index=%d\n", __func__, __LINE__,index);

    return 0;
}



static int ov5640_g_mbus_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct ov5640_priv *priv = to_ov5640(client);
    if (!priv->win || !priv->cfmt) {
        u32 width = 640; //SVGA_WIDTH;
        u32 height = 480 ; //SVGA_HEIGHT;
        int ret = ov5640_set_params(sd, &width, &height, V4L2_MBUS_FMT_UYVY8_2X8);
        if (ret < 0) {
            printk("%s, %d\n", __func__, __LINE__);
            return ret;
        }
    }

    mf->width   = priv->win->width;
    mf->height  = priv->win->height;
    mf->code    = priv->cfmt->code;
    mf->colorspace  = priv->cfmt->colorspace;
    mf->field   = V4L2_FIELD_NONE;

    printk("%s, %d\n", __func__, __LINE__);
    return 0;
}



static int ov5640_try_mbus_fmt(struct v4l2_subdev *sd,
        struct v4l2_mbus_framefmt *mf)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct ov5640_priv *priv = to_ov5640(client);
    const struct ov5640_win_size *win;
    int i;

    /*
     * select suitable win
     */
    win = select_win(mf->width, mf->height);
    if (!win)
        return -EINVAL;

    mf->width   = win->width;
    mf->height  = win->height;
    mf->field   = V4L2_FIELD_NONE;


    for (i = 0; i < ARRAY_SIZE(ov5640_cfmts); i++)
        if (mf->code == ov5640_cfmts[i].code)
            break;

    if (i == ARRAY_SIZE(ov5640_cfmts)) {
        /* Unsupported format requested. Propose either */
        if (priv->cfmt) {
            /* the current one or */
            mf->colorspace = priv->cfmt->colorspace;
            mf->code = priv->cfmt->code;
        } else {
            /* the default one */
            mf->colorspace = ov5640_cfmts[0].colorspace;
            mf->code = ov5640_cfmts[0].code;
        }
    } else {
        /* Also return the colorspace */
        mf->colorspace	= ov5640_cfmts[i].colorspace;
    }


    printk("%s, %d\n", __func__, __LINE__);

    return 0;
}


static int ov5640_s_mbus_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct ov5640_priv *priv = to_ov5640(client);

    int ret = ov5640_set_params(sd, &mf->width, &mf->height, mf->code);

    printk("%s(%d): rest = %d\n", __FUNCTION__, __LINE__, ret);

    if (!ret)
        mf->colorspace = priv->cfmt->colorspace;

    printk("%s....%d  colorspace is %d  code is %d \n",__FUNCTION__,__LINE__,priv->cfmt->colorspace,priv->cfmt->code);

    return ret;



}

static const struct v4l2_subdev_video_ops ov5640_subdev_video_ops = {
    .s_stream               = ov5640_s_stream,//sp2518_s_stream,
    .enum_framesizes        = ov5640_enum_framesizes,//sp2518_enum_framesizes,
    .enum_mbus_fmt          = ov5640_enum_mbus_fmt, //sp2518_enum_mbus_fmt,
    .g_mbus_fmt             = ov5640_g_mbus_fmt, //sp2518_g_mbus_fmt,
    .try_mbus_fmt           = ov5640_try_mbus_fmt, //sp2518_try_mbus_fmt,
    .s_mbus_fmt             = ov5640_s_mbus_fmt, //sp2518_s_mbus_fmt,  //first  call by hal
 };

/**
 * pad ops */
static int ov5640_s_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
        struct v4l2_subdev_format *fmt)
{
    struct v4l2_mbus_framefmt *mf = &fmt->format;
    printk("%s: %dx%d\n", __func__, mf->width, mf->height);

    return ov5640_s_mbus_fmt(sd, mf);



}

static const struct v4l2_subdev_pad_ops ov5640_subdev_pad_ops = {
    .set_fmt    =  ov5640_s_fmt,
};

/**
 * subdev ops
 */
static const struct v4l2_subdev_ops ov5640_subdev_ops = {
    .core   = &ov5640_subdev_core_ops,
    .video  = &ov5640_subdev_video_ops,
    .pad    = &ov5640_subdev_pad_ops,
};

/**
 * media_entity_operations
 */
static int ov5640_link_setup(struct media_entity *entity,
        const struct media_pad *local,
        const struct media_pad *remote, u32 flags)
{
    printk("%s\n", __func__);
    return 0;
}

static const struct media_entity_operations ov5640_media_ops = {
    .link_setup = ov5640_link_setup,
};

/****************************************************************************************
 * initialize
 */


static void ov5640_priv_init(struct ov5640_priv * priv)
{
    priv->model = V4L2_IDENT_AMBIGUOUS;
    priv->prev_capt_mode = PREVIEW_MODE;
 //   priv->timeperframe.denominator = 12;//30;
 //   priv->timeperframe.numerator = 1;
 //   priv->win = &ov5640_wins_vga;
}




static int ov5640_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct ov5640_priv *priv;
    struct v4l2_subdev *sd;
    int ret;

    priv = (struct ov5640_priv *)kzalloc(sizeof(struct ov5640_priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    ov5640_priv_init(priv);

    sd = &priv->subdev;
    strcpy(sd->name, MODULE_NAME);

    /* register subdev */
    v4l2_i2c_subdev_init(sd, client, &ov5640_subdev_ops);

    sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
  //  priv->pad.flags = MEDIA_PAD_FL_SOURCE;
    sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
    sd->entity.ops  = &ov5640_media_ops;
    if (media_entity_init(&sd->entity, 1, &priv->pad, 0)) {
        dev_err(&client->dev, "%s: failed to media_entity_init()\n", __func__);
        kfree(priv);
        return -ENOENT;
    }

    ret = ov5640_initialize_ctrls(priv);
    if (ret < 0) {
        printk(KERN_ERR "%s: failed to initialize controls\n", __func__);
        kfree(priv);
        return ret;
    }

   printk("%s:%d.......\n", __func__, __LINE__);


    return 0;
}

static int ov5640_remove(struct i2c_client *client)
{
    struct v4l2_subdev *sd = i2c_get_clientdata(client);
    v4l2_device_unregister_subdev(sd);
    v4l2_ctrl_handler_free(sd->ctrl_handler);
    media_entity_cleanup(&sd->entity);
    kfree(to_ov5640(client));

    printk("%s: on %d line \n", __func__, __LINE__);

    return 0;
}

static const struct i2c_device_id ov5640_id[] = {
    { MODULE_NAME, 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, ov5640_id);

static struct i2c_driver ov5640_i2c_driver = {
    .driver = {
        .name = MODULE_NAME,
    },
    .probe    = ov5640_probe,
    .remove   = ov5640_remove,
    .id_table = ov5640_id,
};

module_i2c_driver(ov5640_i2c_driver);

MODULE_DESCRIPTION("SoC Camera driver for OV5640");
MODULE_AUTHOR("xunwei_dg@topeet.com)");
MODULE_LICENSE("GPL v2");
