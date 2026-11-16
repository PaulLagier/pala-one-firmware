import {browserLanguage, configureLanguage} from './one-language.js';

const picker = document.getElementById('device-language');
const button = document.getElementById('apply-language');
const status = document.getElementById('language-status');
picker.value = browserLanguage(navigator.languages || [navigator.language || 'en']);
if (!navigator.serial || !window.isSecureContext) {
  button.disabled = true;
  status.textContent = 'USB language setup requires desktop Chrome or Edge over HTTPS.';
}
button.addEventListener('click', async () => {
  button.disabled = true;
  picker.disabled = true;
  try {
    // Explicit user gesture, independent of the installer dialog's connection.
    const port = await navigator.serial.requestPort();
    status.textContent = 'Saving device language…';
    await configureLanguage(port, picker.value);
    status.textContent = 'Language saved. The reader is restarting.';
  } catch (error) {
    status.textContent = error.name === 'NotFoundError'
      ? 'No device selected.'
      : 'Language not confirmed. Close the installer dialog and try again, or use the app or reader web settings.';
  } finally {
    button.disabled = false;
    picker.disabled = false;
  }
});
