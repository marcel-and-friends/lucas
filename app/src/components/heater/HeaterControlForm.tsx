import { Button } from "@/components/ui/button";
import useFormPersist from "react-hook-form-persist";
import {
  Form,
  FormControl,
  FormField,
  FormItem,
  FormLabel,
  FormMessage,
} from "@/components/ui/form";
import { Input } from "@/components/ui/input";
import { zodResolver } from "@hookform/resolvers/zod";
import { Control, useForm } from "react-hook-form";
import { z } from "zod";

export default function HeaterControlForm({ isHeating, onSubmit }: Props) {
  const form = useForm({
    resolver: zodResolver(HeatingParametersSchema),
    disabled: isHeating,
    mode: "onChange",
  });

  useFormPersist("HeaterControlForm", {
    watch: form.watch,
    setValue: form.setValue,
    storage: window.localStorage,
    validate: true,
  });

  return (
    <Form {...form}>
      <form
        onSubmit={form.handleSubmit(onSubmit)}
        className="flex flex-col gap-2"
      >
        <InputField
          control={form.control}
          name="targetTemperature"
          label="Temperatura Target"
          placeholder="94"
        />
        <InputField
          control={form.control}
          name="duration"
          label="Duração"
          placeholder="15"
        />
        <InputField
          control={form.control}
          name="preheatDurationMultiplier"
          label="Multiplicador de tempo do preheat"
          placeholder="30"
        />
        <InputField
          control={form.control}
          name="preheatPowerMultiplier"
          label="Multiplicador de força do preheat"
          placeholder="2"
        />
        <div className="flex gap-2">
          <InputField
            control={form.control}
            name="p"
            label="P"
            placeholder="0.15"
          />
          <InputField
            control={form.control}
            name="i"
            label="I"
            placeholder="0.15"
          />
          <InputField
            control={form.control}
            name="d"
            label="D"
            placeholder="0.15"
          />
        </div>
        <Button
          disabled={!form.formState.isValid}
          type="submit"
          variant={isHeating ? "destructive" : "default"}
        >
          {isHeating ? "Cancelar" : "Aquecer"}
        </Button>
      </form>
    </Form>
  );
}

interface Props {
  isHeating: boolean;
  onSubmit: (params: HeatingParameters) => Promise<void>;
}

function InputField({
  type,
  placeholder,
  name,
  label,
  control,
}: InputFieldProps) {
  return (
    <FormField
      control={control}
      name={name}
      render={({ field }) => (
        <FormItem>
          <FormLabel>{label}</FormLabel>
          <FormControl>
            <Input
              type={type}
              placeholder={placeholder}
              {...field}
              value={field.value ?? ""}
            />
          </FormControl>
          <FormMessage />
        </FormItem>
      )}
    />
  );
}

interface InputFieldProps {
  control: Control<HeatingParameters>;
  name: keyof HeatingParameters;
  label: string;
  placeholder: string;
  type?: string;
}

const HeatingParametersSchema = z.object({
  targetTemperature: z.coerce
    .number()
    .min(1, { message: "Temperatura deve ser ao menos 1°C" })
    .max(100, { message: "Temperatura deve ser no máximo 100°C" }),

  duration: z.coerce
    .number()
    .min(1, { message: "Duração deve ser ao menos 1s" })
    .max(180, { message: "Duração deve ser no máximo 180s" }),

  preheatDurationMultiplier: z.coerce.number(),
  preheatPowerMultiplier: z.coerce.number(),

  p: z.coerce.number(),
  i: z.coerce.number(),
  d: z.coerce.number(),
});

export type HeatingParameters = z.infer<typeof HeatingParametersSchema>;
