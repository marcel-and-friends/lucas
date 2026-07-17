import { IonContent } from "@ionic/react";
import { useState } from "react";
import { AlarmCode } from "#/proto/firmware/Alarm";
import { HeatingStage } from "#/proto/firmware/heater/HeatingReport";
import HeaterControlForm, {
  type HeatingParameters,
} from "@/components/heater/HeaterControlForm";
import HeatingChart, {
  type GraphPoint,
} from "@/components/heater/HeatingChart";
import { Button } from "@/components/ui/button";
import useBridge from "@/hooks/useBridge";
import useFirmwareEvent from "@/hooks/useFirmwareEvent";

const ALARM_MESSAGES: Record<number, string> = {
  [AlarmCode.SENSOR_FAILURE]:
    "Falha no sensor de temperatura — aquecimento desligado pelo firmware",
  [AlarmCode.OVER_TEMPERATURE]:
    "Temperatura máxima do aquecedor excedida — aquecimento desligado pelo firmware",
};

export default function Heater() {
  const bridge = useBridge();

  const [graphData, setGraphData] = useState<GraphPoint[]>([]);
  const [isHeating, setIsHeating] = useState(false);
  const [alarm, setAlarm] = useState<string | null>(null);
  const [targetTemperature, setTargetTemperature] = useState<
    number | undefined
  >(undefined);

  useFirmwareEvent("alarm", (event) => {
    setIsHeating(false);
    setAlarm(ALARM_MESSAGES[event.code] ?? `Alarme desconhecido (${event.code})`);
  });

  useFirmwareEvent("heating_report", (event) => {
    if (event.stage === HeatingStage.Finished) setIsHeating(false);
    setGraphData((temperatures) => [
      ...temperatures,
      {
        temperature:
          event.stage === HeatingStage.PreHeating
            ? undefined
            : event.temperature,
        preheatTemperature:
          event.stage === HeatingStage.PreHeating
            ? event.temperature
            : undefined,
        pid: event.pid === -1 ? undefined : Math.min(event.pid, 100),
        power: event.power,
        secondsElapsed: event.seconds_elapsed,
        stage: event.stage,
      },
    ]);
  });

  const onSubmit = async (params: HeatingParameters) => {
    if (!isHeating) {
      setGraphData([]);
      setIsHeating(true);
      setAlarm(null);
      setTargetTemperature(params.targetTemperature);

      await bridge.sendCommand({
        heater_control: {
          start: {
            target_temperature: params.targetTemperature,
            duration_ms: Math.round(params.duration * 1000),

            preheat_duration_multiplier: params.preheatDurationMultiplier,
            preheat_power_multiplier: params.preheatPowerMultiplier,

            p: params.p,
            i: params.i,
            d: params.d,
          },
        },
      });
    } else {
      setIsHeating(false);
      await bridge.sendCommand({
        heater_control: {
          stop: {},
        },
      });
    }
  };

  return (
    <IonContent>
      <div className="flex h-full w-full flex-col items-center justify-center gap-10 p-3 md:flex-row">
        <div className="flex-5">
          <HeatingChart
            graphData={graphData}
            targetTemperature={targetTemperature}
          />
        </div>
        <div className="flex flex-1 flex-col gap-1.5">
          {alarm && (
            <button
              type="button"
              className="rounded-md bg-red-600 p-3 text-left text-sm font-medium text-white"
              onClick={() => setAlarm(null)}
            >
              {alarm}
            </button>
          )}
          <HeaterControlForm isHeating={isHeating} onSubmit={onSubmit} />
          <Button
            disabled={isHeating}
            onClick={() => {
              setGraphData([]);
              bridge.sendCommand({
                heater_control: {
                  start: {
                    target_temperature: 0,
                    duration_ms: 15000,

                    preheat_duration_multiplier: 0,
                    preheat_power_multiplier: 0,

                    p: 0,
                    i: 0,
                    d: 0,
                  },
                },
              });
            }}
          >
            Despejar agua fria
          </Button>
        </div>
      </div>
    </IonContent>
  );
}
