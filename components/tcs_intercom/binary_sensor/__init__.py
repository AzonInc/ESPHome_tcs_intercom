import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID, CONF_ICON, CONF_LAMBDA
from .. import tcs_intercom_ns, TCSIntercom, CONF_TCS_ID

TCSIntercomBinarySensor = tcs_intercom_ns.class_(
    "TCSIntercomBinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONF_COMMAND = "command"
CONF_NAME = "name"
CONF_AUTO_OFF = "auto_off"

DEPENDENCIES = ["tcs_intercom"]

def validate(config):
    config = config.copy()
    if CONF_LAMBDA in config:
        if CONF_COMMAND in config:
            raise cv.Invalid("command cannot be used with lambda")
    return config

CONFIG_SCHEMA = cv.All(
    binary_sensor.binary_sensor_schema(TCSIntercomBinarySensor).extend(
        {
            cv.GenerateID(): cv.declare_id(TCSIntercomBinarySensor),
            cv.GenerateID(CONF_TCS_ID): cv.use_id(TCSIntercom),
            cv.Optional(CONF_LAMBDA): cv.returning_lambda,
            cv.Optional(CONF_COMMAND): cv.hex_uint32_t,
            cv.Optional(CONF_ICON, default="mdi:doorbell"): cv.icon,
            cv.Optional(CONF_NAME, default="Doorbell"): cv.string,
            cv.Optional(CONF_AUTO_OFF, default="3s"): cv.positive_time_period_milliseconds
        }
    ),
    cv.has_exactly_one_key(CONF_LAMBDA, CONF_COMMAND),
    validate,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await binary_sensor.register_binary_sensor(var, config)

    if CONF_LAMBDA in config:
        template_ = await cg.process_lambda(
            config[CONF_LAMBDA], [], return_type=cg.optional.template(cg.uint32)
        )
        cg.add(var.set_template(template_))

    if CONF_COMMAND in config:
        cg.add(var.set_command(config[CONF_COMMAND]))

    cg.add(var.set_auto_off(config[CONF_AUTO_OFF]))

    tcs_intercom = await cg.get_variable(config[CONF_TCS_ID])
    cg.add(tcs_intercom.register_listener(var))