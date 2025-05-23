import {
  ChartConfig,
  ChartContainer,
  ChartLegend,
  ChartLegendContent,
  ChartTooltip,
  ChartTooltipContent,
} from "@/components/ui/chart";
import {
  CartesianGrid,
  Line,
  LineChart,
  ReferenceLine,
  XAxis,
  YAxis,
} from "recharts";
import { HeatingStage } from "#/proto/firmware/heater/HeatingReport";

const chartConfig = {
  temperature: {
    label: "Temperatura (°C)",
  },
  preheatTemperature: {
    label: "Temperatura no preheat (°C)",
  },
  pid: {
    label: "PID (%)",
  },
  power: {
    label: "Força (%)",
  },
} satisfies ChartConfig;

export default function HeatingChart({ graphData, targetTemperature }: Props) {
  const firstHeating = graphData.findIndex(
    (d) => d.stage == HeatingStage.Heating,
  );

  let heatingPercentage = 0;
  if (firstHeating != -1) {
    heatingPercentage =
      ((graphData.length - firstHeating) * 100) / (graphData.length - 1);
  }

  const tickFormatter = (tick: number) => tick.toFixed(2).replace(/\.?0+$/, "");

  return (
    <ChartContainer config={chartConfig}>
      <LineChart data={graphData}>
        <CartesianGrid vertical={false} strokeDasharray="3" />
        <ReferenceLine y={targetTemperature} stroke="var(--foreground)" />
        <XAxis
          dataKey="secondsElapsed"
          type="number"
          tickFormatter={tickFormatter}
          tickCount={graphData.length}
          allowDecimals={false}
          domain={[0, "dataMax"]}
          label={{
            value: "Tempo (s)",
            position: "insideBottom",
          }}
        />
        <YAxis
          type="number"
          domain={[20, 100]}
          allowDecimals={false}
          tickFormatter={tickFormatter}
        />
        <ChartTooltip
          animationDuration={100}
          content={
            <ChartTooltipContent
              hideLabel
              formatter={(value, name, item) => (
                <>
                  {chartConfig[name as keyof typeof chartConfig]?.label || name}
                  <div className="ml-auto flex items-baseline gap-0.5 font-mono font-medium text-foreground tabular-nums">
                    {tickFormatter(Number(value))}
                    <span className="font-normal text-muted-foreground">
                      {name === "Temperatura" ? "°C" : "%"}
                    </span>
                  </div>
                  {name === "Força" && (
                    <div className="flex basis-full items-center border-t pt-1.5 text-xs font-medium text-foreground">
                      Tempo
                      {item.payload && (
                        <div className="ml-auto flex items-baseline gap-0.5 font-mono font-medium text-foreground tabular-nums">
                          {tickFormatter(item.payload.secondsElapsed)}
                          <span className="font-normal text-muted-foreground">
                            s
                          </span>
                        </div>
                      )}
                    </div>
                  )}
                </>
              )}
            />
          }
        />
        <ChartLegend verticalAlign="top" content={<ChartLegendContent />} />
        <Line
          name="Temperatura"
          dataKey="temperature"
          type="monotone"
          stroke="#8884d8"
          strokeWidth="3px"
          isAnimationActive={false}
          dot={false}
          activeDot={false}
        />
        <Line
          name="Temperatura"
          dataKey="preheatTemperature"
          type="monotone"
          stroke="#f58d42"
          strokeWidth="3px"
          isAnimationActive={false}
          dot={false}
          activeDot={false}
        />
        <Line
          name="PID"
          dataKey="pid"
          type="monotone"
          stroke="#7dff45"
          strokeWidth="3px"
          isAnimationActive={false}
          dot={false}
          activeDot={false}
        />
        <Line
          name="Força"
          dataKey="power"
          type="monotone"
          stroke="#ff5485"
          strokeWidth="3px"
          isAnimationActive={false}
          dot={false}
          activeDot={false}
        />
      </LineChart>
    </ChartContainer>
  );
}

interface Props {
  graphData: GraphPoint[];
  targetTemperature: number | undefined;
}

export interface GraphPoint {
  temperature: number | undefined;
  preheatTemperature: number | undefined;
  pid: number | undefined;
  power: number;
  secondsElapsed: number;
  stage: HeatingStage;
}
