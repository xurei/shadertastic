/**
 * @pattern ^#([A-Fa-f0-9]{8})$
 */
type ColorARGB = `#${string}`;
/**
 * @pattern ^#([A-Fa-f0-9]{6})$
 */
type ColorRGB = `#${string}`;

type param_condition_member = boolean | number | string;
type param_condition_not = {
    "not": Array<param_condition>
};
type param_condition_and = {
    "and": Array<param_condition>
};
type param_condition_or = {
    "or": Array<param_condition>
};
type param_condition_comp = [
    param_condition_member,
    (
        '==' | "eq" |
        "!=" | 'ne' | 'neq' |
        '>'  | 'gt' |
        '>=' | 'gte' |
        '<'  | 'lt' |
        '<=' | 'lte'
    ),
    param_condition_member
];
type param_condition = (
    param_condition_or |
    param_condition_and |
    param_condition_not |
    param_condition_comp
);

type param_common = {
    /**
     * Name of the field. It *must* be the same as the associated variable in the shader.
     */
    name: string,

    /**
     * The label of the field in the UI. If not set, the name will be used.
     *
     * @default Same as name
     */
    label?: string,

    /**
     * Description of the field. It will be shown in the UI.
     */
    description?: string,

    /**
     * Condition for display
     */
    if?: param_condition,

    /**
     * If set to true, this field will only be available in the property UI if
     * the developer mode is active.
     *
     * @default false
     */
    devmode?: boolean,
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * An audio-level aware parameter that responds to audio input.
 *
 * The `audiolevel` type represents a parameter that can monitor and respond to audio levels.
 * This allows for creating effects that react to sound or music.
 *
 * @shaderparam `uniform float`
 *
 * @example ```{
 *   "type": "audiolevel",
 *   "name": "audio_level",
 *   "label": "Audio Level"
 * }```
 */
type param_audiolevel = param_common & {
    type: "audiolevel",
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * Boolean parameter
 *
 * This will show a Yes/No input UI in the effect properties.
 *
 * @shaderparam `uniform bool`
 *
 * @example ```{
 *   "type": "bool",
 *   "name": "my_bool",
 *   "default": true,
 *   "label": "A boolean"
 * }```
 */
type param_bool = param_common & {
    type: "bool",
    default: boolean,
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * Color parameter
 *
 * @shaderparam `uniform float4`
 *
 * @example ```{
 *   "type": "color",
 *   "name": "my_color",
 *   "default": "#99ABCDEF"
 * }```
 */
type param_color = param_common & {
    type: "color",
    /**
     * Format: #RRGGBB or #AARRGGBB
     * If alpha is not present, full opacity will be used
     */
    default: ColorRGB | ColorARGB,
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * Use the face tracking functionnality.
 *
 * @shaderparam `uniform texture2d`
 *
 * @example ```{
 *   "type": "facetracking",
 *   "name": "fd"
 * }```
 */
type param_facetracking = param_common & {
    type: "facetracking",

    /**
     * If set to true, use the face tracking data to build a secondary texture mapping the face.
     * Each color channel has a different meaning :
     * - R: index of the triangle in the face mesh
     * - G,B: Barycentric coordinates of the point, for texturing
     */
    use_preraster?: boolean,
    // /**
    //  * Allow to chain the result of the face tracking to the next filter.
    //  * If enabled, the face tracking algorithm will not be applied again on posterior filters. Instead, the result of
    //  * this filter will be used.
    //  *
    //  * Enable this option only if you don't move the face in the scene with the effect.
    //  * For example, the "Laser Eyes" effect uses this option, but the "Distort" effect doesn't.
    //  *
    //  * @defaut false
    //  */
    // allow_chaining?: boolean,
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * Float/Double parameter
 *
 * The `float` type represents a floating-point parameter that can be exposed in the user interface.
 * It allows for configuring numeric inputs with options like sliders and value constraints.
 *
 * @shaderparam `uniform float`
 *
 * @example ```{
 *   "type": "float",
 *   "name": "my_float_value",
 *   "label": "My float",
 *   "slider": true,
 *   "min": 0.0,
 *   "max": 2.0,
 *   "default": 1.0,
 *   "step": 0.01
 * }```
 */
type param_float = param_common & {
    type: "float" | "double",
    default: number,

    /**
     * Show a slider in the UI
     * @default false
     */
    slider?: boolean,
    /**
     * Minimum value of the parameter
     * @default 0.0
     */
    min?: number,
    /**
     * Maximum value of the parameter
     * @default 100.0
     */
    max?: number,
    /**
     * Smaller step allowed
     * @default 0.01
     */
    step?: number,
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * Float/Double parameter
 *
 * The `float` type represents a floating-point parameter that can be exposed in the user interface.
 * It allows for configuring numeric inputs with options like sliders and value constraints.
 *
 * @shaderparam `uniform float`
 *
 * @example ```{
 *   "type": "float",
 *   "name": "my_float_value",
 *   "label": "My float",
 *   "slider": true,
 *   "min": 0.0,
 *   "max": 2.0,
 *   "default": 1.0,
 *   "step": 0.01
 * }```
 */
type param_group = param_common & {
    type: "group",
    parameters: param[],
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * Image parameter.
 *
 * It can be either a bundled image in your effect (e.g. a font map to draw text),a file input
 * in the filter properties, or both.
 *
 * @shaderparam `uniform texture2d`
 *
 * @example ```{
 *   "name": "grid_pattern",
 *   "label": "Pattern",
 *   "type": "image",
 *   "values": [
 *     {"label": "Pattern1", "value": "pattern1.png"},
 *     {"label": "Pattern2", "value": "pattern2.png"},
 *     {"label": "Pattern3", "value": "pattern3.png"},
 *     {"label": "Pattern4", "value": "pattern4.png"}
 *   ],
 *   "default": "pattern1.png",
 *   "allow_custom": true,
 *   "hidden": false
 * }```
 */
type param_image = param_common & {
    type: "image",
    default: string,

    /**
     * Allow to select an image with a file picker in the effect properties UI.
     * @default true
     */
    allow_custom?: boolean,
    /**
     * Optional list of pre-defined images.
     *
     * These files must be in the same folder of you effect, or in the same packaged .shadertastic file to be valid.
     *
     * If `allowed_custom` is true, another pick "Use custom file" will also be available in the UI.
     * @example ```[
     *   {"label": "Map", "value": "map.png"},
     *   {"label": "Map2", "value": "map2.png"},
     *   {"label": "Map3", "value": "map3.png"},
     *   {"label": "Map4", "value": "map4.png"}
     * ]```
     * @default null
     */
    values?: Array<{ label: string, value: string }>,
    /**
     * Hide the UI field
     *
     * @default false
     */
    hidden?: boolean,
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * Integer parameter
 *
 * The `int` type represents an integer parameter that can be exposed in the user interface.
 * It allows for configuring numeric inputs with options like sliders and value constraints.
 *
 * @shaderparam `uniform int`
 *
 * @example ```{
 *   "type": "int",
 *   "name": "my_int_value",
 *   "label": "My int",
 *   "slider": true,
 *   "min": 0,
 *   "max": 100,
 *   "default": 50,
 *   "step": 1
 * }```
 */
type param_int = param_common & {
    type: "int",
    /** @type integer */
    default: number,

    /**
     * Show a slider in the UI
     * @default false
     */
    slider?: boolean,
    /**
     * Minimum value of the parameter. If not set, 0.0 will be used
     * @type integer
     */
    min?: number,
    /**
     * Maximum value of the parameter. If not set, 100 will be used
     * @type integer
     */
    max?: number,
    /**
     * Smaller step allowed. If not set, 1 will be used
     * @type integer
     * @default 1
     */
    step?: number,
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * List of Integers parameter.
 *
 * The `list_int` shows a select input type in the effect UI.
 * This is typically used to change the behaviour of the effect and making it easier to understand for the end-user.
 *
 * @shaderparam `uniform int`
 *
 * @example ```{
 *   "type": "list_int",
 *   "name": "appear_mode",
 *   "label": "Appear mode",
 *   "values": [
 *     {"label": "Previous scene over next", "value": 0},
 *     {"label": "Next scene over previous", "value": 1}
 *   ],
 *   "default": 0
 * }```
 */
type param_list_int = param_common & {
    type: "list_int",

    /**
     * List of values available
     * @type { label: string, value: integer }f
     */
    values: Array<{
        label: string,
        /** @type integer */
        value: number,
    }>,
    /** @type integer */
    default: number,
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * Save the previous frame of a specific step as a parameter
 *
 * @shaderparam `uniform texture2d`
 *
 * @example ```{
 *   "type": "param_prev_frame",
 *   "name": "prev_frame",
 *   "step": 1
 * }```
 */
type param_prev_frame = param_common & {
    type: "prev_frame",
    /**
     * The step to keep as a previous frame. It must be less or equal to the number of steps defined in the effect root
     * metadata.
     *
     * If set to -1 (the default value), the last step will be used
     *
     * @default -1
     *
     * @type integer
     **/
    step: number,
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * Video Source parameter
 *
 * @shaderparam `uniform texture2d`
 *
 * @example ```{
 *   "type": "source",
 *   "name": "my_source"
 * }```
 */
type param_source = param_common & {
    type: "source",
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * Show some text in the properties UI
 *
 * No shader code associated.
 *
 * @example ```{
 *   "type": "text",
 *   "value": "This text will be shown in the properties UI"
 * }```
 */
type param_text = param_common & {
    type: "text",
    value: string,
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * Execution time of the effect, in seconds.
 * An optional speed input is provided to accelerate the effect.
 * Also, the time can be reset to zero when the filter is toggled as visible
 *
 * @shaderparam `uniform float`
 *
 * @example ```{
 *   "type": "param_time",
 *   "name": "time",
 *   "reset_on_show": "prompt",
 *   "speed": {
 *     "default": 0.1
 *   }
 * }```
 */
type param_time = param_common & {
    type: "time",
    /**
     * If true, the time will be reset to zero when the filter is set visible.
     * If "prompt", let the user decide by showing an input in the property UI.
     * @default false
     */
    reset_on_show?: boolean | "prompt",

    /**
     * Speed options
     */
    speed?: {
        /**
         * If true, the speed slider will be show in the effects properties
         * @default true
         */
        show_ui?: boolean,
        /**
         * If show, name of the input in the properties UI
         * @default "Speed"
         */
        label?: string,
        /**
         * Minimum allowed speed
         * @default 0.0
         */
        min?: number,
        /**
         * Maximum allowed speed
         * @default 1.0
         */
        max?: number,
        /**
         * Default value to use in a new filter
         * @default 1.0
         */
        default?: number,
    }
}
//----------------------------------------------------------------------------------------------------------------------

export type param = (
    param_audiolevel |
    param_bool |
    param_color |
    param_facetracking |
    param_float |
    param_group |
    param_image |
    param_int |
    param_list_int |
    param_prev_frame |
    param_source |
    param_text |
    param_time
);
//----------------------------------------------------------------------------------------------------------------------

export type meta = {
    /**
     * Version of the meta file. Will be used in the future to migrate older meta files and keep the legacy code working
     * @type integer
     * @default 1
     */
    metafileVersion: number,

    /**
     * About information. You can add whatever you want next to the author name and the description of the effect
     */
    about: {
        author_name: string,
        author_url?: string,
        description?: string,
        [key: string]: string,
    },

    /**
     * Name of the effect as shown in the OBS UI
     */
    label: string,

    /**
     * Revision number
     */
    revision?: number,

    /**
     * Number of steps in the effect
     * @default 0
     * @type integer
     */
    steps?: number,

    /**
     * Extra Paramters of the effect
     */
    parameters?: param[],

    /**
     * DEPRECATED Old way of using a time parameter in the effect. Use the parameter `time` instead
     * @deprecated
     */
    input_time?: boolean,

    /**
     * DEPRECATED Old way of using the face detection parameter in the effect. Use the parameter `face_detection` instead
     * @deprecated
     */
    input_facedetection?: boolean,
}
